#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>
#include <TestFramework/Utilities/TestLogInterface.h>

// Since wdOpenDdlReader is implemented by deriving from wdOpenDdlParser, this tests both classes

static void WriteObjectToDDL(const wdOpenDdlReaderElement* pElement, wdOpenDdlWriter& ref_writer)
{
  if (pElement->HasName())
  {
    WD_TEST_BOOL(!wdStringUtils::IsNullOrEmpty(pElement->GetName()));
  }

  if (pElement->IsCustomType())
  {
    ref_writer.BeginObject(pElement->GetCustomType(), pElement->GetName(), pElement->IsNameGlobal());

    wdUInt32 uiChildren = 0;
    auto pChild = pElement->GetFirstChild();
    while (pChild)
    {
      ++uiChildren;

      if (pChild->HasName())
      {
        wdString sNameCopy = pChild->GetName();
        const wdOpenDdlReaderElement* pChild2 = pElement->FindChild(sNameCopy);

        WD_TEST_BOOL(pChild == pChild2);
      }

      WriteObjectToDDL(pChild, ref_writer);
      pChild = pChild->GetSibling();
    }

    ref_writer.EndObject();

    WD_TEST_INT(uiChildren, pElement->GetNumChildObjects());
  }
  else
  {
    const wdOpenDdlPrimitiveType type = pElement->GetPrimitivesType();

    ref_writer.BeginPrimitiveList(type, pElement->GetName(), pElement->IsNameGlobal());

    switch (type)
    {
      case wdOpenDdlPrimitiveType::Bool:
        ref_writer.WriteBool(pElement->GetPrimitivesBool(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::Int8:
        ref_writer.WriteInt8(pElement->GetPrimitivesInt8(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::Int16:
        ref_writer.WriteInt16(pElement->GetPrimitivesInt16(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::Int32:
        ref_writer.WriteInt32(pElement->GetPrimitivesInt32(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::Int64:
        ref_writer.WriteInt64(pElement->GetPrimitivesInt64(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::UInt8:
        ref_writer.WriteUInt8(pElement->GetPrimitivesUInt8(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::UInt16:
        ref_writer.WriteUInt16(pElement->GetPrimitivesUInt16(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::UInt32:
        ref_writer.WriteUInt32(pElement->GetPrimitivesUInt32(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::UInt64:
        ref_writer.WriteUInt64(pElement->GetPrimitivesUInt64(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::Float:
        ref_writer.WriteFloat(pElement->GetPrimitivesFloat(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::Double:
        ref_writer.WriteDouble(pElement->GetPrimitivesDouble(), pElement->GetNumPrimitives());
        break;

      case wdOpenDdlPrimitiveType::String:
      {
        for (wdUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
        {
          ref_writer.WriteString(pElement->GetPrimitivesString()[i]);
        }
      }
      break;

      case wdOpenDdlPrimitiveType::Custom:
      default:
        WD_ASSERT_NOT_IMPLEMENTED;
        break;
    }

    ref_writer.EndPrimitiveList();
  }
}

static void WriteToDDL(const wdOpenDdlReader& doc, wdStreamWriter& ref_output)
{
  wdOpenDdlWriter writer;
  writer.SetOutputStream(&ref_output);
  writer.SetPrimitiveTypeStringMode(wdOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetFloatPrecisionMode(wdOpenDdlWriter::FloatPrecisionMode::Readable);

  const auto* pRoot = doc.GetRootElement();
  WD_ASSERT_DEV(pRoot != nullptr, "Invalid root");

  if (pRoot == nullptr)
    return;

  auto pChild = pRoot->GetFirstChild();
  while (pChild)
  {
    WriteObjectToDDL(pChild, writer);
    pChild = pChild->GetSibling();
  }
}

static void WriteToString(const wdOpenDdlReader& doc, wdStringBuilder& ref_sString)
{
  wdContiguousMemoryStreamStorage storage;
  wdMemoryStreamWriter writer(&storage);

  WriteToDDL(doc, writer);

  wdUInt8 term = 0;
  writer.WriteBytes(&term, 1).IgnoreResult();
  ref_sString = (const char*)storage.GetData();
}

static void TestEqual(const char* szOriginal, const char* szRecreation)
{
  wdUInt32 uiChar = 0;

  do
  {
    const wdUInt8 cOrg = szOriginal[uiChar];
    const wdUInt8 cAlt = szRecreation[uiChar];

    if (cOrg != cAlt)
    {
      WD_TEST_FAILURE("String compare failed", "DDL Original and recreation don't match at character %u ('%c' -> '%c')", uiChar, cOrg, cAlt);
      return;
    }

    ++uiChar;
  } while (szOriginal[uiChar - 1] != '\0');
}

// These functions test the reader by doing a round trip from string -> reader -> writer -> string and then comparing the string to the
// original Therefore the original must be formatted exactly as the writer would format it (mostly regarding indentation, newlines, spaces
// and floats) and may not contain things that get removed (ie. comments)
static void TestDoc(const wdOpenDdlReader& doc, const char* szOriginal)
{
  wdStringBuilder recreation;
  WriteToString(doc, recreation);

  TestEqual(szOriginal, recreation);
}

WD_CREATE_SIMPLE_TEST(IO, DdlReader)
{
  WD_TEST_BLOCK(wdTestBlock::Enabled, "Basics and Comments")
  {
    const char* szTestData = "Node{\
  Name{ string{ \"ConstantColor\" } }\
\
  OutputPins\
  {\
    float %MyFloats{ 1.2, 3, 4e1, .5, 6_0, .7e-2 } \
    double$MyDoubles{1.2,3,4e1,.5,6_0,.7e-2} \
    int8{0,1/**/27,,  ,128  , -127/*comment*/ , -128,  -129}\
    string{ \"float4\" }\
    bool{ true, false , true,, false }\
  }\
\
  Properties\
  {\
    Property\
    {\
      Name{ string{ \"Color\" } }\
      Type{ string{ \"color\" } }\
    }\
  }\
}\
// some comment \
";

    StringStream stream(szTestData);

    wdOpenDdlReader doc;
    WD_TEST_BOOL(doc.ParseDocument(stream).Succeeded());
    WD_TEST_BOOL(!doc.HadFatalParsingError());

    auto pRoot = doc.GetRootElement();
    WD_TEST_INT(pRoot->GetNumChildObjects(), 1);
  }

  WD_TEST_BLOCK(wdTestBlock::Enabled, "Structure")
  {
    const char* szTestData = "Node\n\
{\n\
	Name\n\
	{\n\
		string{\"ConstantColor\"}\n\
	}\n\
	OutputPins\n\
	{\n\
		float %MyFloats{1.2,3,40,0.5,60}\n\
		double $MyDoubles{1.2,3,40,0.5,60}\n\
		int8{0,127,32,-127,-127}\n\
		string{\"float4\"}\n\
		bool{true,false,true,false}\n\
	}\n\
	Properties\n\
	{\n\
		Property\n\
		{\n\
			Name\n\
			{\n\
				string{\"Color\"}\n\
			}\n\
			Type\n\
			{\n\
				string{\"color\"}\n\
			}\n\
		}\n\
	}\n\
}\n\
";

    StringStream stream(szTestData);

    wdOpenDdlReader doc;
    WD_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    auto pElement = doc.FindElement("MyDoubles");
    WD_TEST_BOOL(pElement != nullptr);
    if (pElement)
    {
      WD_TEST_STRING(pElement->GetName(), "MyDoubles");
    }

    WD_TEST_BOOL(doc.FindElement("MyFloats") == nullptr);
    WD_TEST_BOOL(doc.FindElement("Node") == nullptr);

    TestDoc(doc, szTestData);
    WD_TEST_BOOL(!doc.HadFatalParsingError());
  }

  WD_TEST_BLOCK(wdTestBlock::Enabled, "All Primitives")
  {
    const char* szTestData = "\
bool{true,false,true,true,false}\n\
string{\"s1\",\"\\n\\t\\r\"}\n\
float{0,1.1,-3,23.42}\n\
double{0,1.1,-3,23.42}\n\
int8{0,12,34,56,78,109,127,-14,-56,-127}\n\
int16{0,102,3040,5600,7008,109,10207,-1004,-5060,-10207}\n\
int32{0,100002,300040,56000000,700008,1000009,100000207,-100000004,-506000000,-1020700000}\n\
int64{0,100002111,300040222,560000003333,70000844444,1000009555555,100000207666666,-1000000047777777,-50600000008888888,-102070000099999}\n\
unsigned_int8{0,12,34,56,78,109,127,255,156,207}\n\
unsigned_int16{0,102,3040,56000,7008,109,10207,40004,50600,10207}\n\
unsigned_int32{0,100002,300040,56000000,700008,1000009,100000207,100000004,2000001000,1020700000}\n\
unsigned_int64{0,100002111,300040222,560000003333,70000844444,1000009555555,100000207666666,1000000047777777,50600000008888888,102070000099999}\n\
";

    StringStream stream(szTestData);

    wdOpenDdlReader doc;
    WD_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    TestDoc(doc, szTestData);
    WD_TEST_BOOL(!doc.HadFatalParsingError());
  }

  WD_TEST_BLOCK(wdTestBlock::Enabled, "Errors")
  {
    const char* szTestData = "\
string{\"s1\",\"back\\slash\"}\n\
string{\"s\\2\",\"bla\"}\n\
";

    StringStream stream(szTestData);

    wdTestLogInterface log;
    wdTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Unknown escape-sequence '\\s'", wdLogMsgType::WarningMsg);
    log.ExpectMessage("Unknown escape-sequence '\\2'", wdLogMsgType::WarningMsg);

    wdOpenDdlReader doc;
    WD_TEST_BOOL(doc.ParseDocument(stream).Succeeded()); // no fatal error
  }

  WD_TEST_BLOCK(wdTestBlock::Enabled, "Fatal Errors")
  {
    const char* szTestData = "\
string{\"s1\",\"back\\slash\"\n\
string{\"s\\2\",\"bla\"}\n\
";

    StringStream stream(szTestData);

    wdTestLogInterface log;
    wdTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Unknown escape-sequence '\\s'", wdLogMsgType::WarningMsg);
    log.ExpectMessage("Line 2 (2): Expected , or } or a \"", wdLogMsgType::ErrorMsg);

    wdOpenDdlReader doc;
    WD_TEST_BOOL(doc.ParseDocument(stream).Failed());
  }
}
