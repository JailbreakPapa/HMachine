#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/Id.h>

#define WD_MSVC_WARNING_NUMBER 4463
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

struct TestId
{
  typedef wdUInt32 StorageType;

  WD_DECLARE_ID_TYPE(TestId, 20, 6);

  WD_ALWAYS_INLINE TestId(StorageType instanceIndex, StorageType generation, StorageType systemIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation = generation;
    m_SystemIndex = systemIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 20;
      StorageType m_Generation : 6;
      StorageType m_SystemIndex : 6;
    };
  };
};

typedef wdGenericId<32, 10> LargeTestId;

WD_CREATE_SIMPLE_TEST(Basics, Id)
{
  TestId id1;
  WD_TEST_INT(id1.m_InstanceIndex, TestId::INVALID_INSTANCE_INDEX);
  WD_TEST_INT(id1.m_Generation, 0);
  WD_TEST_INT(id1.m_SystemIndex, 0);

  TestId id2(1, 20, 15);
  TestId id3(1, 84, 79); // overflow
  WD_TEST_INT(id2.m_InstanceIndex, 1);
  WD_TEST_INT(id2.m_Generation, 20);
  WD_TEST_INT(id2.m_SystemIndex, 15);
  WD_TEST_BOOL(id2 == id3);

  id2.m_InstanceIndex = 2;
  WD_TEST_INT(id2.m_InstanceIndex, 2);
  WD_TEST_BOOL(id2 != id3);
  WD_TEST_BOOL(!id2.IsIndexAndGenerationEqual(id3));

  id2.m_InstanceIndex = 1;
  id2.m_SystemIndex = 16;
  WD_TEST_BOOL(id2 != id3);
  WD_TEST_BOOL(id2.IsIndexAndGenerationEqual(id3));

  id2.m_Generation = 94; // overflow
  WD_TEST_INT(id2.m_Generation, 30);

  id2.m_SystemIndex = 94; // overflow
  WD_TEST_INT(id2.m_SystemIndex, 30);

  LargeTestId id4(1, 1224); // overflow
  WD_TEST_INT(id4.m_InstanceIndex, 1);
  WD_TEST_INT(id4.m_Generation, 200);
}
