#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

const char* wdInputManager::ConvertScanCodeToEngineName(wdUInt8 uiScanCode, bool bIsExtendedKey)
{
  const wdUInt8 uiFinalScanCode = bIsExtendedKey ? (uiScanCode + 128) : uiScanCode;

  switch (uiFinalScanCode)
  {
    case 1:
      return wdInputSlot_KeyEscape;
    case 2:
      return wdInputSlot_Key1;
    case 3:
      return wdInputSlot_Key2;
    case 4:
      return wdInputSlot_Key3;
    case 5:
      return wdInputSlot_Key4;
    case 6:
      return wdInputSlot_Key5;
    case 7:
      return wdInputSlot_Key6;
    case 8:
      return wdInputSlot_Key7;
    case 9:
      return wdInputSlot_Key8;
    case 10:
      return wdInputSlot_Key9;
    case 11:
      return wdInputSlot_Key0;
    case 12:
      return wdInputSlot_KeyHyphen;
    case 13:
      return wdInputSlot_KeyEquals;
    case 14:
      return wdInputSlot_KeyBackspace;
    case 15:
      return wdInputSlot_KeyTab;
    case 16:
      return wdInputSlot_KeyQ;
    case 17:
      return wdInputSlot_KeyW;
    case 18:
      return wdInputSlot_KeyE;
    case 19:
      return wdInputSlot_KeyR;
    case 20:
      return wdInputSlot_KeyT;
    case 21:
      return wdInputSlot_KeyY;
    case 22:
      return wdInputSlot_KeyU;
    case 23:
      return wdInputSlot_KeyI;
    case 24:
      return wdInputSlot_KeyO;
    case 25:
      return wdInputSlot_KeyP;
    case 26:
      return wdInputSlot_KeyBracketOpen;
    case 27:
      return wdInputSlot_KeyBracketClose;
    case 28:
      return wdInputSlot_KeyReturn;
    case 29:
      return wdInputSlot_KeyLeftCtrl;
    case 30:
      return wdInputSlot_KeyA;
    case 31:
      return wdInputSlot_KeyS;
    case 32:
      return wdInputSlot_KeyD;
    case 33:
      return wdInputSlot_KeyF;
    case 34:
      return wdInputSlot_KeyG;
    case 35:
      return wdInputSlot_KeyH;
    case 36:
      return wdInputSlot_KeyJ;
    case 37:
      return wdInputSlot_KeyK;
    case 38:
      return wdInputSlot_KeyL;
    case 39:
      return wdInputSlot_KeySemicolon;
    case 40:
      return wdInputSlot_KeyApostrophe;
    case 41:
      return wdInputSlot_KeyTilde;
    case 42:
      return wdInputSlot_KeyLeftShift;
    case 43:
      return wdInputSlot_KeyBackslash;
    case 44:
      return wdInputSlot_KeyZ;
    case 45:
      return wdInputSlot_KeyX;
    case 46:
      return wdInputSlot_KeyC;
    case 47:
      return wdInputSlot_KeyV;
    case 48:
      return wdInputSlot_KeyB;
    case 49:
      return wdInputSlot_KeyN;
    case 50:
      return wdInputSlot_KeyM;
    case 51:
      return wdInputSlot_KeyComma;
    case 52:
      return wdInputSlot_KeyPeriod;
    case 53:
      return wdInputSlot_KeySlash;
    case 54:
      return wdInputSlot_KeyRightShift;
    case 55:
      return wdInputSlot_KeyNumpadStar;
    case 56:
      return wdInputSlot_KeyLeftAlt;
    case 57:
      return wdInputSlot_KeySpace;
    case 58:
      return wdInputSlot_KeyCapsLock;
    case 59:
      return wdInputSlot_KeyF1;
    case 60:
      return wdInputSlot_KeyF2;
    case 61:
      return wdInputSlot_KeyF3;
    case 62:
      return wdInputSlot_KeyF4;
    case 63:
      return wdInputSlot_KeyF5;
    case 64:
      return wdInputSlot_KeyF6;
    case 65:
      return wdInputSlot_KeyF7;
    case 66:
      return wdInputSlot_KeyF8;
    case 67:
      return wdInputSlot_KeyF9;
    case 68:
      return wdInputSlot_KeyF10;
    case 69:
      return wdInputSlot_KeyNumLock;
    case 70:
      return wdInputSlot_KeyScroll;
    case 71:
      return wdInputSlot_KeyNumpad7;
    case 72:
      return wdInputSlot_KeyNumpad8;
    case 73:
      return wdInputSlot_KeyNumpad9;
    case 74:
      return wdInputSlot_KeyNumpadMinus;
    case 75:
      return wdInputSlot_KeyNumpad4;
    case 76:
      return wdInputSlot_KeyNumpad5;
    case 77:
      return wdInputSlot_KeyNumpad6;
    case 78:
      return wdInputSlot_KeyNumpadPlus;
    case 79:
      return wdInputSlot_KeyNumpad1;
    case 80:
      return wdInputSlot_KeyNumpad2;
    case 81:
      return wdInputSlot_KeyNumpad3;
    case 82:
      return wdInputSlot_KeyNumpad0;
    case 83:
      return wdInputSlot_KeyNumpadPeriod;


    case 86:
      return wdInputSlot_KeyPipe;
    case 87:
      return wdInputSlot_KeyF11;
    case 88:
      return wdInputSlot_KeyF12;


    case 91:
      return wdInputSlot_KeyLeftWin;
    case 92:
      return wdInputSlot_KeyRightWin;
    case 93:
      return wdInputSlot_KeyApps;



    case 128 + 16:
      return wdInputSlot_KeyPrevTrack;
    case 128 + 25:
      return wdInputSlot_KeyNextTrack;
    case 128 + 28:
      return wdInputSlot_KeyNumpadEnter;
    case 128 + 29:
      return wdInputSlot_KeyRightCtrl;
    case 128 + 32:
      return wdInputSlot_KeyMute;
    case 128 + 34:
      return wdInputSlot_KeyPlayPause;
    case 128 + 36:
      return wdInputSlot_KeyStop;
    case 128 + 46:
      return wdInputSlot_KeyVolumeDown;
    case 128 + 48:
      return wdInputSlot_KeyVolumeUp;
    case 128 + 53:
      return wdInputSlot_KeyNumpadSlash;
    case 128 + 55:
      return wdInputSlot_KeyPrint;
    case 128 + 56:
      return wdInputSlot_KeyRightAlt;
    case 128 + 70:
      return wdInputSlot_KeyPause;
    case 128 + 71:
      return wdInputSlot_KeyHome;
    case 128 + 72:
      return wdInputSlot_KeyUp;
    case 128 + 73:
      return wdInputSlot_KeyPageUp;
    case 128 + 75:
      return wdInputSlot_KeyLeft;
    case 128 + 77:
      return wdInputSlot_KeyRight;
    case 128 + 79:
      return wdInputSlot_KeyEnd;
    case 128 + 80:
      return wdInputSlot_KeyDown;
    case 128 + 81:
      return wdInputSlot_KeyPageDown;
    case 128 + 82:
      return wdInputSlot_KeyInsert;
    case 128 + 83:
      return wdInputSlot_KeyDelete;

    default:

      // for extended keys fall back to the non-extended name
      if (bIsExtendedKey)
        return ConvertScanCodeToEngineName(uiScanCode, false);

      break;
  }

  return "unknown_key";
}



WD_STATICLINK_FILE(Core, Core_Input_Implementation_ScancodeTable);
