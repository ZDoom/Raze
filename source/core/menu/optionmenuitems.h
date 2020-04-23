/*
** optionmenuitems.h
** Control items for option menus
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include "v_text.h"
#include "v_draw.h"
#include "gstrings.h"
#include "v_font.h"
#include "cmdlib.h"


void M_DrawConText (int color, int x, int y, const char *str);
void M_SetVideoMode();



//=============================================================================
//
// opens a submenu, action is a submenu name
//
//=============================================================================

class FOptionMenuItemSubmenu : public FOptionMenuItem
{
	int mParam;
public:
	FOptionMenuItemSubmenu(const char *label, const char *menu, int param = 0)
		: FOptionMenuItem(label, menu)
	{
		mParam = param;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColorMore);
		return indent;
	}

	bool Activate(FName caller) override
	{
		M_MenuSound(CursorSound);
		M_SetMenu(mAction, mParam, static_cast<DOptionMenu*>(DMenu::CurrentMenu)->GetDescriptor()->mMenuName);
		return true;
	}
};


//=============================================================================
//
// opens a submenu, command is a submenu name
//
//=============================================================================

class FOptionMenuItemLabeledSubmenu : public FOptionMenuItemSubmenu
{
	FBaseCVar *mLabelCVar;
public:
	FOptionMenuItemLabeledSubmenu(const char * label, FBaseCVar *labelcvar, FName command, int param = 0)
	 : FOptionMenuItemSubmenu(label, command.GetChars(), param)
	{
		mLabelCVar = labelcvar;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor);
		
		auto text = mLabelCVar? mLabelCVar->GetHumanString() : "";
		if (text[0] == 0) text = GStrings("notset");
		drawValue(indent, y, OptionSettings.mFontColorValue, text);
		return indent;
	}
};


//=============================================================================
//
// Executes a CCMD, action is a CCMD name
//
//=============================================================================

class FOptionMenuItemCommand : public FOptionMenuItemSubmenu
{
public:
	FOptionMenuItemCommand(const char *label, const char *menu)
		: FOptionMenuItemSubmenu(label, menu)
	{
	}

	bool Activate(FName caller) override
	{
		M_MenuSound(AdvanceSound);
		C_DoCommand(mAction.GetChars());
		return true;
	}

};

//=============================================================================*
//
// Executes a CCMD after confirmation, action is a CCMD name
//
//=============================================================================

class FOptionMenuItemSafeCommand : public FOptionMenuItemCommand
{
	// action is a CCMD
	FString mPrompt;
	int mScriptId;	// shouldn't be used, but just in case.
public:
	FOptionMenuItemSafeCommand(const char *label, const char *menu, const char *prompt = nullptr, int scriptid = INT_MAX)
		: FOptionMenuItemCommand(label, menu)
	{
		mPrompt = prompt;
		mScriptId = scriptid;
	}

	bool MenuEvent (int mkey, bool fromcontroller) override
	{
		if (mkey == MKEY_MBYes)
		{
			C_DoCommand(mAction.GetChars());
			return true;
		}
		return FOptionMenuItemCommand::MenuEvent(mkey, fromcontroller);
	}

	bool Activate(FName caller) override
	{
		FString msg;
		msg << GStrings.localize(mPrompt.IsNotEmpty()? mPrompt.GetChars() : "$SAFEMESSAGE") << "\n\n" << GStrings.localize("$PRESSYN");
		auto actionLabel = GStrings.localize(mLabel.GetChars());

		//FStringf FullString("%s%s%s\n\n%s", TEXTCOLOR_WHITE, actionLabel, TEXTCOLOR_NORMAL, msg);
		FStringf FullString("- %s -\n%s", actionLabel, msg.GetChars());
		M_StartMessage(FullString, 0, mScriptId);
		return true;
	}
};

//=============================================================================
//
// Base class for option lists
//
//=============================================================================

class FOptionMenuItemOptionBase : public FOptionMenuItem
{
protected:
	// action is a CVAR
	FName mValues;	// Entry in OptionValues table
	FBaseCVar *mGrayCheck;
	int mCenter;
public:

	enum
	{
		OP_VALUES = 0x11001
	};

	FOptionMenuItemOptionBase(const char *label, const char *menu, const char *values, const char *graycheck, int center)
		: FOptionMenuItem(label, menu)
	{
		mValues = values;
		mGrayCheck = (FBoolCVar*)FindCVar(graycheck, NULL);
		mCenter = center;
	}

	bool SetString(int i, const char *newtext) override
	{
		if (i == OP_VALUES) 
		{
			FOptionValues **opt = OptionValues.CheckKey(newtext);
			mValues = newtext;
			if (opt != NULL && *opt != NULL) 
			{
				int s = GetSelection();
				if (s >= (int)(*opt)->mValues.Size()) s = 0;
				SetSelection(s);	// readjust the CVAR if its value is outside the range now
				return true;
			}
		}
		return false;
	}



	//=============================================================================
	virtual int GetSelection() = 0;
	virtual void SetSelection(int Selection) = 0;

	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		if (mCenter)
		{
			indent = (twod->GetWidth() / 2);
		}
		drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor, isGrayed());

		int Selection = GetSelection();
		const char *text;
		FOptionValues **opt = OptionValues.CheckKey(mValues);
		if (Selection < 0 || opt == NULL || *opt == NULL)
		{
			text = "Unknown";
		}
		else
		{
			text = (*opt)->mValues[Selection].Text;
		}
		if (*text == '$') text = GStrings(text + 1);

		drawValue(indent, y, OptionSettings.mFontColorValue, text, isGrayed());
		return indent;
	}

	//=============================================================================
	bool MenuEvent (int mkey, bool fromcontroller) override
	{
		FOptionValues **opt = OptionValues.CheckKey(mValues);
		if (opt != NULL && *opt != NULL && (*opt)->mValues.Size() > 0)
		{
			int Selection = GetSelection();
			if (mkey == MKEY_Left)
			{
				if (Selection == -1) Selection = 0;
				else if (--Selection < 0) Selection = (*opt)->mValues.Size()-1;
			}
			else if (mkey == MKEY_Right || mkey == MKEY_Enter)
			{
				if (++Selection >= (int)(*opt)->mValues.Size()) Selection = 0;
			}
			else
			{
				return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
			}
			SetSelection(Selection);
			M_MenuSound(ChangeSound);
		}
		return true;
	}

	virtual bool isGrayed()
	{
		return mGrayCheck != NULL && !(mGrayCheck->GetGenericRep(CVAR_Bool).Bool);
	}

	bool Selectable() override
	{
		return !isGrayed();
	}
};

//=============================================================================
//
// Change a CVAR, action is the CVAR name
//
//=============================================================================

class FOptionMenuItemOption : public FOptionMenuItemOptionBase
{
	// action is a CVAR
	FBaseCVar *mCVar;
public:

	FOptionMenuItemOption(const char *label, const char *menu, const char *values, const char *graycheck, int center)
		: FOptionMenuItemOptionBase(label, menu, values, graycheck, center)
	{
		mCVar = FindCVar(mAction.GetChars(), NULL);
	}

	//=============================================================================
	int GetSelection() override
	{
		int Selection = -1;
		FOptionValues **opt = OptionValues.CheckKey(mValues);
		if (opt != NULL && *opt != NULL && mCVar != NULL && (*opt)->mValues.Size() > 0)
		{
			if ((*opt)->mValues[0].TextValue.IsEmpty())
			{
				UCVarValue cv = mCVar->GetGenericRep(CVAR_Float);
				for(unsigned i = 0; i < (*opt)->mValues.Size(); i++)
				{ 
					if (fabs(cv.Float - (*opt)->mValues[i].Value) < FLT_EPSILON)
					{
						Selection = i;
						break;
					}
				}
			}
			else
			{
				UCVarValue cv = mCVar->GetGenericRep(CVAR_String);
				for(unsigned i = 0; i < (*opt)->mValues.Size(); i++)
				{
					if ((*opt)->mValues[i].TextValue.CompareNoCase(cv.String) == 0)
					{
						Selection = i;
						break;
					}
				}
			}
		}
		return Selection;
	}

	void SetSelection(int Selection) override
	{
		UCVarValue value;
		FOptionValues **opt = OptionValues.CheckKey(mValues);
		if (opt != NULL && *opt != NULL && mCVar != NULL && (*opt)->mValues.Size() > 0)
		{
			if ((*opt)->mValues[0].TextValue.IsEmpty())
			{
				value.Float = (float)(*opt)->mValues[Selection].Value;
				mCVar->SetGenericRep (value, CVAR_Float);
			}
			else
			{
				value.String = (*opt)->mValues[Selection].TextValue.LockBuffer();
				mCVar->SetGenericRep (value, CVAR_String);
				(*opt)->mValues[Selection].TextValue.UnlockBuffer();
			}
		}
	}
};

//=============================================================================
//
// This class is used to capture the key to be used as the new key binding
// for a control item
//
//=============================================================================

class DEnterKey : public DMenu
{
	int *pKey;

public:
	DEnterKey(DMenu *parent, int *keyptr)
	: DMenu(parent)
	{
		pKey = keyptr;
		SetMenuMessage(1);
		menuactive = MENU_WaitKey;	// There should be a better way to disable GUI capture...
	}

	bool TranslateKeyboardEvents() override
	{
		return false; 
	}

	void SetMenuMessage(int which)
	{
		DOptionMenu *m = static_cast<DOptionMenu*>(mParentMenu);
		if (m)
		{
			FListMenuItem *it = m->GetItem(NAME_Controlmessage);
			if (it != NULL)
			{
				it->SetValue(0, which);
			}
		}
	}

	bool Responder(event_t *ev) override
	{
		if (ev->type == EV_KeyDown)
		{
			*pKey = ev->data1;
			menuactive = MENU_On;
			SetMenuMessage(0);
			auto p = mParentMenu;
			Close();
			p->MenuEvent((ev->data1 == KEY_ESCAPE)? MKEY_Abort : MKEY_Input, 0);
			return true;
		}
		return false;
	}

	void Drawer() override
	{
		mParentMenu->Drawer();
	}
};

//=============================================================================
//
// // Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

class FOptionMenuItemControl : public FOptionMenuItem
{
	FKeyBindings *mBindings;
	int mInput;
	bool mWaiting;
public:

	FOptionMenuItemControl(const char *label, const char *menu, FKeyBindings *bindings)
		: FOptionMenuItem(label, menu)
	{
		mBindings = bindings;
		mWaiting = false;
	}


	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		drawLabel(indent, y, mWaiting? OptionSettings.mFontColorHighlight:
			(selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor));

		auto binds = mBindings->GetKeysForCommand(mAction.GetChars());
		auto description = C_NameKeys(binds.Data(), binds.Size());

		if (description.Len() > 0)
		{
			drawValue(indent, y, CR_WHITE, description);
		}
		else
		{
			drawValue(indent, y, CR_BLACK, "---");
		}
		return indent;
	}

	//=============================================================================
	bool MenuEvent(int mkey, bool fromcontroller) override
	{
		if (mkey == MKEY_Input)
		{
			mWaiting = false;
			mBindings->SetBind(mInput, mAction.GetChars());
			return true;
		}
		else if (mkey == MKEY_Clear)
		{
			mBindings->UnbindACommand(mAction.GetChars());
			return true;
		}
		else if (mkey == MKEY_Abort)
		{
			mWaiting = false;
			return true;
		}
		return false;
	}

	bool Activate(FName caller) override
	{
		M_MenuSound(AdvanceSound);
		mWaiting = true;
		DMenu *input = new DEnterKey(DMenu::CurrentMenu, &mInput);
		M_ActivateMenu(input);
		return true;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuItemStaticText : public FOptionMenuItem
{
	EColorRange mColor;
public:
	FOptionMenuItemStaticText(const char *label, EColorRange color = CR_UNDEFINED)
		: FOptionMenuItem(label, NAME_None, true)
	{
		mColor = color == CR_UNDEFINED? (EColorRange)OptionSettings.mFontColor : color;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		drawLabel(indent, y, mColor);
		return -1;
	}

	bool Selectable() override
	{
		return false;
	}

};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuItemStaticTextSwitchable : public FOptionMenuItem
{
	EColorRange mColor;
	FString mAltText;
	int mCurrent;

public:
	FOptionMenuItemStaticTextSwitchable(const char *label, const char *label2, FName action, EColorRange color = CR_UNDEFINED)
		: FOptionMenuItem(label, action, true)
	{
		mColor = color == CR_UNDEFINED? (EColorRange)OptionSettings.mFontColor : color;
		mAltText = label2;
		mCurrent = 0;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		const char *txt = GStrings.localize(mCurrent? mAltText.GetChars() : mLabel.GetChars());
		int w = OptionWidth(txt) * CleanXfac_1;
		int x = (twod->GetWidth() - w) / 2;
		drawText(x, y, mColor, txt);
		return -1;
	}

	bool SetValue(int i, int val) override
	{
		if (i == 0) 
		{
			mCurrent = val;
			return true;
		}
		return false;
	}

	bool SetString(int i, const char *newtext) override
	{
		if (i == 0) 
		{
			mAltText = newtext;
			return true;
		}
		return false;
	}

	bool Selectable() override
	{
		return false;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderBase : public FOptionMenuItem
{
	// action is a CVAR
	double mMin, mMax, mStep;
	int mShowValue;
	int mDrawX;
	int mSliderShort;

public:
	FOptionMenuSliderBase(const char *label, double min, double max, double step, int showval, FName command = NAME_None)
		: FOptionMenuItem(label, command)
	{
		mMin = min;
		mMax = max;
		mStep = step;
		mShowValue = showval;
		mDrawX = 0;
		mSliderShort = 0;
	}

	virtual double GetSliderValue() = 0;
	virtual void SetSliderValue(double val) = 0;

	//=============================================================================
	//
	// Draw a slider. Set fracdigits negative to not display the current value numerically.
	//
	//=============================================================================
	
	void DrawSliderElement (int color, int x, int y, const char * str)
	{
		DrawText (twod, ConFont, color, x, y, str, DTA_CellX, 16 * CleanXfac_1, DTA_CellY, 16 * CleanYfac_1, TAG_DONE);
	}

	void DrawSlider (int x, int y, double min, double max, double cur, int fracdigits, int indent)
	{
		char textbuf[16];
		double range;
		int maxlen = 0;
		int right = x + (12*16 + 4) * CleanXfac_1;
		int cy = y;// +(OptionSettings.mLinespacing - 8) * CleanYfac_1;

		range = max - min;
		double ccur = clamp(cur, min, max) - min;

		if (fracdigits >= 0)
		{
			snprintf(textbuf, countof(textbuf), "%.*f", fracdigits, max);
			maxlen = NewSmallFont->StringWidth(textbuf) * CleanXfac_1;
		}

		mSliderShort = right + maxlen > twod->GetWidth();

		if (!mSliderShort)
		{
			DrawSliderElement(CR_WHITE, x, cy, "\x10\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x12");
			DrawSliderElement(CR_ORANGE, x + int((5 + ((ccur * 78) / range)) * 2*CleanXfac_1), cy, "\x13");
		}
		else
		{
			// On 320x200 we need a shorter slider
			DrawSliderElement(CR_WHITE, x, cy, "\x10\x11\x11\x11\x11\x11\x12");
			DrawSliderElement(CR_ORANGE, x + int((5 + ((ccur * 38) / range)) * 2*CleanXfac_1), cy, "\x13");
			right -= 5*8*CleanXfac_1;
		}

		if (fracdigits >= 0 && right + maxlen <= twod->GetWidth())
		{
			snprintf(textbuf, countof(textbuf), "%.*f", fracdigits, cur);
			drawText(right, y, CR_DARKGRAY, textbuf);
		}
	}


	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected) override
	{
		drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor);
		mDrawX = indent + CursorSpace();
		DrawSlider (mDrawX, y, mMin, mMax, GetSliderValue(), mShowValue, indent);
		return indent;
	}

	//=============================================================================
	bool MenuEvent (int mkey, bool fromcontroller) override
	{
		double value = GetSliderValue();

		if (mkey == MKEY_Left)
		{
			value -= mStep;
		}
		else if (mkey == MKEY_Right)
		{
			value += mStep;
		}
		else
		{
			return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
		}
		SetSliderValue(clamp(value, mMin, mMax));
		M_MenuSound(ChangeSound);
		return true;
	}

	bool MouseEvent(int type, int x, int y) override
	{
		DOptionMenu *lm = static_cast<DOptionMenu*>(DMenu::CurrentMenu);
		if (type != DMenu::MOUSE_Click)
		{
			if (!lm->CheckFocus(this)) return false;
		}
		if (type == DMenu::MOUSE_Release)
		{
			lm->ReleaseFocus();
		}

		int slide_left = mDrawX+16*CleanXfac_1;
		int slide_right = slide_left + (10*16*CleanXfac_1 >> mSliderShort);	// 10 char cells with 8 pixels each.

		if (type == DMenu::MOUSE_Click)
		{
			if (x < slide_left || x >= slide_right) return true;
		}

		x = clamp(x, slide_left, slide_right);
		double v = mMin + ((x - slide_left) * (mMax - mMin)) / (slide_right - slide_left);
		if (v != GetSliderValue())
		{
			SetSliderValue(v);
			M_MenuSound(ChangeSound);
		}
		if (type == DMenu::MOUSE_Click)
		{
			lm->SetFocus(this);
		}
		return true;
	}

};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderCVar : public FOptionMenuSliderBase
{
	FBaseCVar *mCVar;
public:
	FOptionMenuSliderCVar(const char *label, const char *menu, double min, double max, double step, int showval)
		: FOptionMenuSliderBase(label, min, max, step, showval)
	{
		mCVar = FindCVar(menu, NULL);
	}

	double GetSliderValue() override
	{
		if (mCVar != NULL)
		{
			return mCVar->GetGenericRep(CVAR_Float).Float;
		}
		else
		{
			return 0;
		}
	}

	void SetSliderValue(double val) override
	{
		if (mCVar != NULL)
		{
			UCVarValue value;
			value.Float = (float)val;
			mCVar->SetGenericRep(value, CVAR_Float);
		}
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderVar : public FOptionMenuSliderBase
{
	float *mPVal;
public:

	FOptionMenuSliderVar(const char *label, float *pVal, double min, double max, double step, int showval)
		: FOptionMenuSliderBase(label, min, max, step, showval)
	{
		mPVal = pVal;
	}

	double GetSliderValue() override
	{
		return *mPVal;
	}

	void SetSliderValue(double val) override
	{
		*mPVal = (float)val;
	}
};


//=============================================================================
//
// [TP] FOptionMenuFieldBase
//
// Base class for input fields
//
//=============================================================================

class FOptionMenuFieldBase : public FOptionMenuItem
{
public:
	FOptionMenuFieldBase ( const char* label, const char* menu, const char* graycheck ) :
		FOptionMenuItem ( label, menu ),
		mCVar ( FindCVar( mAction.GetChars(), NULL )),
		mGrayCheck (( graycheck && strlen( graycheck )) ? FindCVar( graycheck, NULL ) : NULL ) {}

	const char* GetCVarString()
	{
		if ( mCVar == NULL )
			return "";

		return mCVar->GetGenericRep( CVAR_String ).String;
	}

	virtual FString Represent()
	{
		return GetCVarString();
	}

	int Draw ( FOptionMenuDescriptor*, int y, int indent, bool selected ) override
	{
		bool grayed = mGrayCheck != NULL && !( mGrayCheck->GetGenericRep( CVAR_Bool ).Bool );
		drawLabel(indent, y, selected ? OptionSettings.mFontColorSelection : OptionSettings.mFontColor, grayed);
		drawValue(indent, y, OptionSettings.mFontColorValue, Represent(), grayed);
		return indent;
	}

	bool GetString ( int i, char* s, int len ) override
	{
		if ( i == 0 )
		{
			strncpy( s, GetCVarString(), len );
			s[len - 1] = '\0';
			return true;
		}

		return false;
	}

	bool SetString ( int i, const char* s ) override
	{
		if ( i == 0 )
		{
			if ( mCVar )
			{
				UCVarValue vval;
				vval.String = s;
				mCVar->SetGenericRep( vval, CVAR_String );
			}

			return true;
		}

		return false;
	}

protected:
	// Action is a CVar in this class and derivatives.
	FBaseCVar* mCVar;
	FBaseCVar* mGrayCheck;
};

//=============================================================================
//
// [TP] FOptionMenuTextField
//
// A text input field widget, for use with string CVars.
//
//=============================================================================

class FOptionMenuTextField : public FOptionMenuFieldBase
{
public:
	FOptionMenuTextField ( const char *label, const char* menu, const char* graycheck ) :
		FOptionMenuFieldBase ( label, menu, graycheck ),
		mEntering ( false ) {}

	FString Represent() override
	{
		if (mEntering)
		{
			FString text;
			text = mInput->GetText();
			text += '_';
			return text;
		}
		else return FString(GetCVarString());
	}

	int Draw(FOptionMenuDescriptor*desc, int y, int indent, bool selected) override
	{
		if (mEntering)
		{
			// reposition the text so that the cursor is visible when in entering mode.
			FString text = Represent();
			int tlen = NewSmallFont->StringWidth(text) * CleanXfac_1;
			int newindent = twod->GetWidth() - tlen - CursorSpace();
			if (newindent < indent) indent = newindent;
		}
		return FOptionMenuFieldBase::Draw(desc, y, indent, selected);
	}

	bool MenuEvent ( int mkey, bool fromcontroller ) override
	{
		if ( mkey == MKEY_Enter )
		{
			M_MenuSound(AdvanceSound);
			mEntering = true;
			mInput = new DTextEnterMenu(DMenu::CurrentMenu, NewSmallFont, GetCVarString(), 256, fromcontroller );
			M_ActivateMenu( mInput );
			return true;
		}
		else if ( mkey == MKEY_Input )
		{
			if ( mCVar )
			{
				UCVarValue vval;
				vval.String = mInput->GetText();
				mCVar->SetGenericRep( vval, CVAR_String );
			}

			mEntering = false;
			mInput = nullptr;
			return true;
		}
		else if ( mkey == MKEY_Abort )
		{
			mEntering = false;
			mInput = nullptr;
			return true;
		}

		return FOptionMenuItem::MenuEvent( mkey, fromcontroller );
	}

private:
	bool mEntering;
	DTextEnterMenu* mInput;
};

//=============================================================================
//
// [TP] FOptionMenuNumberField
//
// A numeric input field widget, for use with number CVars where sliders are inappropriate (i.e.
// where the user is interested in the exact value specifically)
//
//=============================================================================

class FOptionMenuNumberField : public FOptionMenuFieldBase
{
public:
	FOptionMenuNumberField ( const char *label, const char* menu, float minimum, float maximum,
		float step, const char* graycheck )
		: FOptionMenuFieldBase ( label, menu, graycheck ),
		mMinimum ( minimum ),
		mMaximum ( maximum ),
		mStep ( step )
	{
		if ( mMaximum <= mMinimum )
			std::swap( mMinimum, mMaximum );

		if ( mStep <= 0 )
			mStep = 1;
	}

	bool MenuEvent ( int mkey, bool fromcontroller ) override
	{
		if ( mCVar )
		{
			float value = mCVar->GetGenericRep( CVAR_Float ).Float;

			if ( mkey == MKEY_Left )
			{
				value -= mStep;

				if ( value < mMinimum )
					value = mMaximum;
			}
			else if ( mkey == MKEY_Right || mkey == MKEY_Enter )
			{
				value += mStep;

				if ( value > mMaximum )
					value = mMinimum;
			}
			else
				return FOptionMenuItem::MenuEvent( mkey, fromcontroller );

			UCVarValue vval;
			vval.Float = value;
			mCVar->SetGenericRep( vval, CVAR_Float );
			M_MenuSound(ChangeSound);
		}

		return true;
	}

private:
	float mMinimum;
	float mMaximum;
	float mStep;
};
