/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

// Custom game options and configuration dialog

#pragma once

#include "C4Gui.h"

// options dialog: created as listbox inside another dialog
// used to configure some standard runtime options, as well as custom game options
class C4GameOptionsList : public C4GUI::ListBox
{
public:
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

private:
	class Option : public C4GUI::Control
	{
	protected:
		typedef C4GUI::Control BaseClass;

		// primary subcomponent: forward focus to this element
		C4GUI::Control *pPrimarySubcomponent;

		virtual bool IsFocused(C4GUI::Control *pCtrl) override
		{
			// also forward own focus to primary control
			return BaseClass::IsFocused(pCtrl) || (HasFocus() && pPrimarySubcomponent == pCtrl);
		}

	public:
		Option(C4GameOptionsList *pForDlg); // adds to list
		void InitOption(C4GameOptionsList *pForDlg); // add to list and do initial update

		virtual void Update() {} // update data

		Option *GetNext() { return static_cast<Option *>(BaseClass::GetNext()); }
	};

	// dropdown list option
	class OptionDropdown : public Option
	{
	public:
		OptionDropdown(C4GameOptionsList *pForDlg, const char *szCaption, bool fReadOnly);

	protected:
		C4GUI::Label *pCaption;
		C4GUI::ComboBox *pDropdownList;

		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) = 0;

		void OnDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
		{
			DoDropdownFill(pFiller);
		}

		virtual void DoDropdownSelChange(int32_t idNewSelection) = 0;

		bool OnDropdownSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
		{
			DoDropdownSelChange(idNewSelection); Update(); return true;
		}
	};

	// drop down list to specify central/decentral control mode
	class OptionControlMode : public OptionDropdown
	{
	public:
		OptionControlMode(C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) override;
		virtual void DoDropdownSelChange(int32_t idNewSelection) override;

		virtual void Update() override; // update data to current control rate
	};

	// drop down list option to adjust control rate
	class OptionControlRate : public OptionDropdown
	{
	public:
		OptionControlRate(C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) override;
		virtual void DoDropdownSelChange(int32_t idNewSelection) override;

		virtual void Update() override; // update data to current control rate
	};

	// drop down list option to adjust team usage
	class OptionTeamDist : public OptionDropdown
	{
	public:
		OptionTeamDist(C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) override;
		virtual void DoDropdownSelChange(int32_t idNewSelection) override;

		virtual void Update() override; // update data to current team mode
	};

	// drop down list option to adjust team color state
	class OptionTeamColors : public OptionDropdown
	{
	public:
		OptionTeamColors(C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) override;
		virtual void DoDropdownSelChange(int32_t idNewSelection) override;

		virtual void Update() override; // update data to current team color mode
	};

	// drop down list option to adjust control rate
	class OptionRuntimeJoin : public OptionDropdown
	{
	public:
		OptionRuntimeJoin(C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) override;
		virtual void DoDropdownSelChange(int32_t idNewSelection) override;

		virtual void Update() override; // update data to current runtime join state
	};

public:
	C4GameOptionsList(const C4Rect &rcBounds, bool fActive, bool fRuntime);
	~C4GameOptionsList() { Deactivate(); }

private:
	C4Sec1TimerCallback<C4GameOptionsList> *pSec1Timer; // engine timer hook for updates
	bool fRuntime; // set for runtime options dialog - does not provide pre-game options such as team colors

	void InitOptions(); // creates option selection components

public:
	// update all option flags by current game state
	void Update();
	void OnSec1Timer() { Update(); }

	// activate/deactivate periodic updates
	void Activate();
	void Deactivate();

	// config
	bool IsTabular() const { return fRuntime; } // wide runtime dialog allows tabular layout
	bool IsRuntime() const { return fRuntime; }
};
