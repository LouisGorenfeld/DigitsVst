/*
 *  EditorWindow.h
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 7/29/12.
 *  Copyright 2012 ExtentOfTheJam. All rights reserved.
 *
 */

#ifndef _EDITWINDOW_H_
#define _EDITWINDOW_H_

#include "../vstgui/plugin-bindings/aeffguieditor.h"
#include "VstCore.h" // kNumParameters

class EditorWindow : public AEffGUIEditor, public IControlListener
{
public:
	EditorWindow(void *p);
	~EditorWindow();

	// AEffGUIEditor
	virtual bool open (void* ptr);
	void close ();
	void setParameter (VstInt32 index, float value);
	void idle();
	
	// CControlListener
	void valueChanged (CControl* pControl);
	
	// The guts of the effect
	VstCore *m_me;
	
protected:
	enum oscType_t
	{
		kOsc1,
		kOsc2,
		kReso
	};
	
	enum tripleType_t
	{
		kMix,
		kAmpEnv,
		kShpEnv
	};
	
	static const int kEditorWidth = 890;
	static const int kEditorHeight = 440;
	// Padding between the edge of a panel and the first
	// control (e.g., from left side of panel to first control)
	static const int kSidePadding = 20;
	static const int kDialPadding = 50;
    static const int kDialWidth = 32;
	static const int kDialSpacing = kDialWidth + 16;
	
	// How far beneath the dial is the text?
	static const int kDialTextPadding = 0;
	// Position text 10 pixels beneath dial
	static const int kTextPaddingY = 50;
	
	static const int kPulldownWidth = 64;
	static const int kTextboxHeight = 14;
	static const int kTextboxWidth = 48;
	
    static const int kBankSelectorKey = 65536;
    static const int kPatchSelectorKey = 65537;
    
    static const int kGUIParamsStartAt = 1000;
    static const int kLFOParams = 8;  // 8 parameters per LFO

    enum
    {
        kGUIParam_LFOButton1 = kGUIParamsStartAt,
        kGUIParam_LFOButton2,
        kGUIParam_LFOButton3,
        kGUIParam_VelButton,
        kGUIParam_ModButton,
        kGUIParam_ATButton,
        kGUIParam_DelButton,
        kGUIParam_ChrButton,
        kGUIParam_BitButton,
        kGUIParam_SaveEnter,
        kGUIParam_SaveOK,
        kGUIParam_SaveCancel
    };
    
    enum velATMod_t
    {
        kVel = 0,
        kAT,
        kMod
    };
    
    int m_curLFO;
    
	CControl* m_controls[VstCore::kNumParams];
    CControl* m_LFOButtons[3];
    CControl* m_velButton;
    CControl* m_atButton;
    CControl* m_modButton;
    CControl* m_velLabels[2];
    CControl* m_atLabels[2];
    CControl* m_modLabels[2];
    
	CTextLabel* m_statusText;
    COptionMenu* m_patchMenu;
    COptionMenu* m_bankMenu;
    CTextEdit *m_saveFilenameField;
    CFrame *m_frame;
    CViewContainer *m_savePopup;
    
    CViewContainer *m_delayView;
    CViewContainer *m_chorusView;
    CViewContainer *m_bitcrushView;
    CControl *m_delayButton;
    CControl *m_chorusButton;
    CControl *m_bitcrushButton;
    
    CViewContainer *m_vuL;
    CViewContainer *m_vuR;
    
    std::vector<CFrame*> m_lines;
    
	// When the status text is set, this is set to the number of idle() calls
	// until the status text goes blank again
	int m_statusTimeout;
    
#ifdef DIGITS_PRO
    // Which LFO is currently shown?
    int m_activeLFO;
#endif
	
	// Draw a panel with skew, a pulldown menu for waveshape, and a label
	void DrawOscPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
					  int x, int y, oscType_t type);

	void DrawEnvPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
							 int x, int y, bool isAmpEnv);

    void DrawMixPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
                             int x, int y);

	void DrawPhasePanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
					   int x, int y);
    
    // Show one LFO panel, hide the rest. whichLFO is the LFO #, starting from 0.
    void ShowLFOPanel(int whichLFO);

    // whichLFO is the number, starting from 0, of the LFO you want drawn
	void DrawLFOPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
					   int x, int y, int whichLFO);

    // Show one FX panel, hide the rest.
    void ShowFXPanel(int which);
    
	void DrawFXPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
					 int x, int y);

    void ShowVelATModPanel(velATMod_t mode);
    
	void DrawVelocityAftertouchPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
					 int x, int y, velATMod_t forMode);
	
	void DrawMiscPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
						   int x, int y);

    void DrawFilterPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
                      int x, int y);

    void DrawMonoPolyPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
                         int x, int y);

    void DrawPatchPanel(CFrame *destFrame, int x, int y);
    // Update the guts of the patch panel, which also updates the banks and patches
    // cache. Set bankChanged to true if changing banks.
    void UpdatePatchPanel(bool bankChanged=false);

    // Add a dial, spit back the handle for the label (handle for dial is in m_controls)
	CTextLabel *AddDial(CViewContainer* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
				  int x, int y, int paramNum, const char* label, float defaultValue=0);

    // Draw our snazzy backgroud
    void DrawLines(CFrame *destFrame);

    void SavePreset(std::string &fn);
};

#endif

