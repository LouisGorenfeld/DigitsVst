/*
 *  EditorWindow.cpp
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 7/29/12.
 *  Copyright 2012 Extent of the Jam. All rights reserved.
 *
 */

#include <algorithm>	// transform
#include "EditorWindow.h"
#include "PatchBankList.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

EditorWindow::EditorWindow(void* p) :
	AEffGUIEditor(p),
    m_me((VstCore*)p),
    m_patchMenu(0),
    m_bankMenu(0)
{
	// Big WTF: This isn't documented, but many hosts
	// call getRect before open. If the window size
	// isn't set first, it won't be correct.
	rect.top = 0;
	rect.left = 0;
	rect.bottom = kEditorHeight;
	rect.right = kEditorWidth;
	
	for (int i=0; i<VstCore::kNumParams; i++)
		m_controls[i] = 0;
}

EditorWindow::~EditorWindow()
{
}

AEffGUIEditor* createEditor (AudioEffectX* effect)
{
	return new EditorWindow (effect);
}

CTextLabel *EditorWindow::AddDial(CViewContainer* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
							int x, int y, int paramNum, const char* caption, float defaultValue)
{
    // These two variables are used interchangeably, so they must be the same
    assert(kDialWidth == dialImg->getWidth());
    
	CRect rDial = CRect(x, y, x+dialImg->getHeight(), y+dialImg->getWidth());
	CRect rText = CRect(x - dialImg->getWidth()*.4, y + dialImg->getHeight() + kDialTextPadding, 
						x + dialImg->getWidth()*1.4, y + dialImg->getHeight() + kDialTextPadding + kTextboxHeight);
	CColor bgColor = CColor(0, 0xD4, 0xB7, 0);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
	
	CKnob* knob = new CKnob(rDial, this, paramNum, dialImg, dialHandleImg);
	knob->setDefaultValue(defaultValue);
	destFrame->addView(knob);

	CTextLabel* label = new CTextLabel(rText, caption);
	label->setBackColor(bgColor);
	label->setFontColor(fgColor);
	label->setFrameColor(bgColor);
	label->setHoriAlign(kCenterText);
	destFrame->addView(label);
	
	// Add knob to list of controls so when params change we can update it
	m_controls[paramNum] = knob;

    return label;
}

void EditorWindow::DrawOscPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
								int x, int y, oscType_t type)
{
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
	CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);

	int key;
	const char *skewText = "Skew";
	const char *resoVolText = "Vol";
	const char *labelText;

	if (type == kOsc1)
	{
		key = VstCore::kO1Skew;
		labelText = skewText;
	}
	else if (type == kOsc2)
	{
		key = VstCore::kO2Skew;
		labelText = skewText;
	}
	else if (type == kReso)
	{
		key = VstCore::kResoVol;
		labelText = resoVolText;
	}
    else
    {
        key = 0;
        labelText = 0;
        assert(false);
    }
	
	AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding, y + kSidePadding, key, labelText, 0);
	
	{
		// Base height off of dial height (has to line up)
		CRect r = CRect(x + kSidePadding + kDialSpacing, 
						y + kSidePadding,
						x + kSidePadding + kDialSpacing + kPulldownWidth,
						y + kSidePadding + dialImg->getHeight());
		
		if (type == kOsc1)
		{
			key = VstCore::kO1ShpMod;
		}
		else if (type == kOsc2)
		{
			key = VstCore::kO2ShpMod;
		}
		else if (type == kReso)
		{
			key = VstCore::kResoWave;
		}
		
		COptionMenu* menu = new COptionMenu(r, this, key);
		if (type == kReso)
		{
			menu->addEntry("Saw Qrtr");
			menu->addEntry("Saw Half");
			menu->addEntry("Sqr Qrtr");
			menu->addEntry("Sqr Half");
		}
		else 
		{
			menu->addEntry("Square");
			menu->addEntry("Saw");
			menu->addEntry("Angry");
			menu->addEntry("Angrier");
		}
		
		menu->setFontColor(bgColorSolid);
		menu->setBackColor(fgColor);
		menu->setFrameColor(bgColor);
		destFrame->addView(menu);
		m_controls[key] = menu;

		{
			CRect rLabel = CRect(x + kSidePadding + kDialSpacing, 
								 y + kSidePadding + dialImg->getHeight(),
								 x + kSidePadding + kDialSpacing + kPulldownWidth,
								 y + kSidePadding + dialImg->getHeight() + kTextboxHeight);
			
			CTextLabel* label = new CTextLabel(rLabel, "Waveshape");
			label->setBackColor(bgColor);
			label->setFontColor(fgColor);
			label->setFrameColor(bgColor);
			label->setHoriAlign(kCenterText);
			destFrame->addView(label);
		}
		
		{
			const char *o1Text = "Oscs";
			const char *o2Text = "Osc2";
			const char *resoText = "Reso";
						
			CRect rLabel = CRect(x, 
								 y,
								 x + kTextboxWidth,
								 y + kTextboxHeight);
			
			if (type == kOsc1)
				labelText = o1Text;
			else if (type == kOsc2)
				labelText = o2Text;
			else if (type == kReso)
				labelText = resoText;
			
			CTextLabel* label = new CTextLabel(rLabel, labelText);
			label->setBackColor(fgColor);
			label->setFontColor(bgColorSolid);
			label->setFrameColor(fgColor);
			label->setHoriAlign(kCenterText);

            if (type != kOsc2)
                destFrame->addView(label);
		}
		
	}
}

void EditorWindow::DrawMixPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
                                int x, int y)
{
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);
    CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor vuRed = CColor(0xFF, 0x20, 0x20);
    CColor vuGreen = CColor(0x20, 0xFF, 0x20);
    
    static const int frameWidth = 200;
    static const int xOffset = (frameWidth - (kDialSpacing*2 + kDialWidth)) / 2;
    static const int rowSpacing = 50;
    
    AddDial(destFrame, dialImg, dialHandleImg, x + xOffset, y + kSidePadding, VstCore::kO2Vol, "A/B", .5);
    AddDial(destFrame, dialImg, dialHandleImg, x + xOffset + kDialSpacing, y + kSidePadding, VstCore::kDetune, "Detune", 0);
    AddDial(destFrame, dialImg, dialHandleImg, x + xOffset + kDialSpacing*2, y + kSidePadding, VstCore::kOctave, "Coarse", .25);

    AddDial(destFrame, dialImg, dialHandleImg, x + xOffset, y + kSidePadding + rowSpacing, VstCore::kOrch, "Phat");
    AddDial(destFrame, dialImg, dialHandleImg, x + xOffset + kDialSpacing, y + kSidePadding + rowSpacing, VstCore::kFM, "FM");
    AddDial(destFrame, dialImg, dialHandleImg, x + xOffset + kDialSpacing*2, y + kSidePadding + rowSpacing, VstCore::kGain, "Gain", .707f /* -3db */);

    CRect rVU = CRect(x + xOffset + kDialSpacing*3,
                      y + kSidePadding,
                      x + xOffset + 10 + kDialSpacing*3,
                      y + kSidePadding + 100);
    m_vuL = new CViewContainer(rVU);
    m_vuL->setBackgroundColor(bgColorSolid);
    destFrame->addView(m_vuL);
    
    CRect rLabel = CRect(x,
                         y,
                         x + kTextboxWidth,
                         y + kTextboxHeight);
    
    CTextLabel* label = new CTextLabel(rLabel, "Mix");
    label->setBackColor(fgColor);
    label->setFontColor(bgColorSolid);
    label->setFrameColor(fgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);
}

void EditorWindow::DrawEnvPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
									   int x, int y, bool isAmpEnv)
{
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
    CColor bgColor2 = CColor(0xCB, 0xD4, 0xB7);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);

	int key;
	const char *labelText;
	float defaultValue = 0;
	
	if (isAmpEnv)
	{
		key = VstCore::kAmpAtt;
		labelText = "Attack";
	}
	else
	{
		key = VstCore::kShpAtt;
		labelText = "Attack";
	}
	
	AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding, y + kSidePadding, key, labelText, defaultValue);

	if (isAmpEnv)
	{
		key = VstCore::kAmpDec;
		labelText = "Decay";
		defaultValue = 0;
	}
	else
	{
		key = VstCore::kShpDec;
		labelText = "Decay";
		defaultValue = .5f;
	}
	
	AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing, y + kSidePadding, key, labelText, defaultValue);

	if (isAmpEnv)
	{
		key = VstCore::kAmpSus;
		labelText = "Sustain";
		defaultValue = 1.0f;
	}
	else
	{
		key = VstCore::kShpSus;
		labelText = "Sustain";
		defaultValue = 0;
	}
	
	AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing*2, y + kSidePadding, key, labelText, defaultValue);

    // Add the 4th and 5th knobs for envelope types
    if (isAmpEnv)
    {
        key = VstCore::kAmpRel;
        labelText = "Release";
        AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing*3, y + kSidePadding, key, labelText, defaultValue);

        key = VstCore::kAmpFade;
        labelText = "Fade";
        AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing*4, y + kSidePadding, key, labelText, defaultValue);

    }
    else
    {
        key = VstCore::kShpRel;
        labelText = "Release";
        AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing*3, y + kSidePadding, key, labelText, defaultValue);
        
        key = VstCore::kShpFade;
        labelText = "Fade";
        AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing*4, y + kSidePadding, key, labelText, defaultValue);
    }

    
    // Draw the sliders!
    CBitmap* sliderBGImg = new CBitmap("HSliderBG.png");
    CBitmap* sliderNubImg = new CBitmap("HSliderNub.png");
    int slidersOffY = 60;
    int slidersOffX = 60;

    if (!isAmpEnv)
	{
		CRect rSlider = CRect(slidersOffX+x+kSidePadding,
							  y+kSidePadding+slidersOffY,
							  slidersOffX+x+kSidePadding+sliderBGImg->getWidth(),
							  y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
        CHorizontalSlider* slider = new CHorizontalSlider(rSlider, this, VstCore::kShpMin,
														  slidersOffX+x+kSidePadding, 
														  slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
														  sliderNubImg, sliderBGImg);
		slider->setStyle(kHorizontal);
		slider->setDefaultValue(0);
		destFrame->addView(slider);
		m_controls[VstCore::kShpMin] = slider;

        
		CRect rLabel = CRect(x+kSidePadding-8,
							 y+kSidePadding+slidersOffY,
							 x+kSidePadding+slidersOffX-8,
							 y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
		
		CTextLabel* label = new CTextLabel(rLabel, "Shp.Min");
		label->setBackColor(bgColor);
		label->setFontColor(fgColor);
		label->setFrameColor(bgColor);
		label->setHoriAlign(kRightText);
		destFrame->addView(label);
		
		rSlider.offset(0, 25);
		rLabel.offset(0, 25);
		
		slider = new CHorizontalSlider(rSlider, this, VstCore::kShpMax,
                                                      slidersOffX+x+kSidePadding,
                                                      slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
                                                      sliderNubImg, sliderBGImg);
		slider->setStyle(kHorizontal);
		slider->setDefaultValue(1.0f);
		destFrame->addView(slider);
		m_controls[VstCore::kShpMax] = slider;
				
		label = new CTextLabel(rLabel, "Shp.Max");
		label->setBackColor(bgColor);
		label->setFontColor(fgColor);
		label->setFrameColor(bgColor);
		label->setHoriAlign(kRightText);
		destFrame->addView(label);
        

        rSlider.offset(0, 25);
        rLabel.offset(0, 25);

        slider = new CHorizontalSlider(rSlider, this, VstCore::kShpRateScale,
                                       slidersOffX+x+kSidePadding,
                                       slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
                                       sliderNubImg, sliderBGImg);
        slider->setStyle(kHorizontal);
        slider->setDefaultValue(0);
        destFrame->addView(slider);
        m_controls[VstCore::kShpRateScale] = slider;

        label = new CTextLabel(rLabel, "R.Scale");
        label->setBackColor(bgColor);
        label->setFontColor(fgColor);
        label->setFrameColor(bgColor);
        label->setHoriAlign(kRightText);
        destFrame->addView(label);
        
        rSlider.offset(0, 25);
        rLabel.offset(0, 25);
        
        slider = new CHorizontalSlider(rSlider, this, VstCore::kShpLevelScale,
                                       slidersOffX+x+kSidePadding,
                                       slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
                                       sliderNubImg, sliderBGImg);
        slider->setStyle(kHorizontal);
        slider->setDefaultValue(0);
        destFrame->addView(slider);
        m_controls[VstCore::kShpLevelScale] = slider;
        
        label = new CTextLabel(rLabel, "L.Scale");
        label->setBackColor(bgColor);
        label->setFontColor(fgColor);
        label->setFrameColor(bgColor);
        label->setHoriAlign(kRightText);
        destFrame->addView(label);

		
		
		sliderNubImg->forget();
		sliderBGImg->forget();
	}
    else
    {
        CRect rSlider = CRect(slidersOffX+x+kSidePadding,
                              y+kSidePadding+slidersOffY,
                              slidersOffX+x+kSidePadding+sliderBGImg->getWidth(),
                              y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
        CHorizontalSlider* slider = new CHorizontalSlider(rSlider, this, VstCore::kAmpRateScale,
                                                          slidersOffX+x+kSidePadding,
                                                          slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
                                                          sliderNubImg, sliderBGImg);
        slider->setStyle(kHorizontal);
        slider->setDefaultValue(0);
        destFrame->addView(slider);
        m_controls[VstCore::kAmpRateScale] = slider;

        CRect rLabel = CRect(x+kSidePadding-8,
                             y+kSidePadding+slidersOffY,
                             x+kSidePadding+slidersOffX-8,
                             y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
        
        CTextLabel* label = new CTextLabel(rLabel, "R.Scale");
        label->setBackColor(bgColor);
        label->setFontColor(fgColor);
        label->setFrameColor(bgColor);
        label->setHoriAlign(kRightText);
        destFrame->addView(label);
    }
	
    CRect rLabel = CRect(x,
                         y,
                         x + kTextboxWidth,
                         y + kTextboxHeight);

	if (isAmpEnv)
		labelText = "AmpEnv";
	else
		labelText = "ShpEnv";
	
	CTextLabel* label = new CTextLabel(rLabel, labelText);
	label->setBackColor(fgColor);
	label->setFontColor(bgColor2);
	label->setFrameColor(fgColor);
	label->setHoriAlign(kCenterText);
	destFrame->addView(label);
}

void EditorWindow::DrawPhasePanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
									   int x, int y)
{
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);
	// The panel on the bottom is 188px, and this is 124px. Indent by 32px to
	// center it
    static const int kIndent = 0;
    
	CBitmap* cosSinImg = new CBitmap("CosSinRockerV.png");
	CRect rRocker(x+kSidePadding+kIndent, y+kSidePadding, 
				  x+kSidePadding+kIndent+cosSinImg->getWidth(), 
				  y+kSidePadding+31);
	CMovieButton* rocker = new CMovieButton(rRocker, this, VstCore::kBasisWave, cosSinImg);	
	destFrame->addView(rocker);
	cosSinImg->forget();
	
		
	{
		CRect rLabel = CRect(x + kSidePadding + kIndent, 
							 y + kSidePadding + dialImg->getHeight(),
							 x + kSidePadding + 57 + kIndent,		// 57 was guesswork -- looked good
							 y + kSidePadding + dialImg->getHeight() + kTextboxHeight);
		
		CTextLabel* label = new CTextLabel(rLabel, "BasisWave");
		label->setBackColor(bgColor);
		label->setFontColor(fgColor);
		label->setFrameColor(bgColor);
		label->setHoriAlign(kCenterText);
		destFrame->addView(label);
	}
	
	AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + 76 + kIndent, y + kSidePadding, VstCore::kPulseWidth, "DutyCycle", .5f);
	
		
    {
//        CCheckBox (const CRect& size, IControlListener* listener = 0, int32_t tag = -1, UTF8StringPtr title = 0, CBitmap* bitmap = 0, int32_t style = 0);
        CRect rCheck = CRect(x + kSidePadding,
                             y + kSidePadding + dialImg->getHeight(),
                             x + kSidePadding + kPulldownWidth,
                             y + kSidePadding + dialImg->getHeight() + kTextboxHeight + 60); // TODO: +60?
        
        CCheckBox *check = new CCheckBox(rCheck, this, VstCore::kResetOscs, "Reset");
        check->setBoxFillColor(bgColorSolid);
        check->setBoxFrameColor(fgColor);
        check->setCheckMarkColor(fgColor);
        check->setFontColor(fgColor);
        destFrame->addView(check);
    }
    
/*    CRect rLabel = CRect(x,
                         y,
                         x + kTextboxWidth,
                         y + kTextboxHeight);

	CTextLabel* label = new CTextLabel(rLabel, "Phase");
	label->setBackColor(bgColor);
	label->setFontColor(fgColor);
	label->setFrameColor(fgColor);
	label->setHoriAlign(kCenterText);
	destFrame->addView(label);*/
    
}

void EditorWindow::ShowVelATModPanel(velATMod_t mode)
{
    if (mode == kAT)
    {
        m_velButton->setValue(0);
        m_atButton->setValue(1.0);
        m_modButton->setValue(0);
        m_controls[VstCore::kVelShaper]->setVisible(false);
        m_controls[VstCore::kVelVolume]->setVisible(false);
        m_controls[VstCore::kModShaper]->setVisible(false);
        m_controls[VstCore::kModLFO1Amp]->setVisible(false);
        m_controls[VstCore::kVelVolume]->setVisible(false);
        m_controls[VstCore::kAftertouchLFO1Amp]->setVisible(true);
        m_controls[VstCore::kAftertouchShaper]->setVisible(true);
        for (int i=0; i < 2; i++)
        {
            m_velLabels[i]->setVisible(false);
            m_atLabels[i]->setVisible(true);
            m_modLabels[i]->setVisible(false);
        }
    }
    else if (mode == kMod)
    {
        m_velButton->setValue(0);
        m_atButton->setValue(0);
        m_modButton->setValue(1.0);
        m_controls[VstCore::kVelShaper]->setVisible(false);
        m_controls[VstCore::kVelVolume]->setVisible(false);
        m_controls[VstCore::kModShaper]->setVisible(true);
        m_controls[VstCore::kModLFO1Amp]->setVisible(true);
        m_controls[VstCore::kVelVolume]->setVisible(false);
        m_controls[VstCore::kAftertouchLFO1Amp]->setVisible(false);
        m_controls[VstCore::kAftertouchShaper]->setVisible(false);
        for (int i=0; i < 2; i++)
        {
            m_velLabels[i]->setVisible(false);
            m_atLabels[i]->setVisible(true);
            m_modLabels[i]->setVisible(false);
        }
    }
    else if (mode == kVel)
    {
        m_velButton->setValue(1.0);
        m_atButton->setValue(0);
        m_modButton->setValue(0);
        m_controls[VstCore::kVelShaper]->setVisible(true);
        m_controls[VstCore::kVelVolume]->setVisible(true);
        m_controls[VstCore::kAftertouchLFO1Amp]->setVisible(false);
        m_controls[VstCore::kAftertouchShaper]->setVisible(false);
        m_controls[VstCore::kModShaper]->setVisible(false);
        m_controls[VstCore::kModLFO1Amp]->setVisible(false);
        for (int i=0; i < 2; i++)
        {
            m_velLabels[i]->setVisible(true);
            m_atLabels[i]->setVisible(false);
            m_modLabels[i]->setVisible(false);
        }
    }
    
#if defined(__i386__) && __APPLE__
    // Force screen redraw (ugly hack)
    m_velButton->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
    m_atButton->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
}

void EditorWindow::ShowLFOPanel(int whichLFO)
{
#ifdef DIGITS_PRO
    assert(whichLFO < Tables::kNumLFOs);
#else
    assert(whichLFO == 0);
#endif
    
    for (int i=0; i<Tables::kNumLFOs; i++)
    {
        bool show = (i == whichLFO);
        int baseOffset = VstCore::kLFORate + (i * kLFOParams);
        
        for (int j=0; j<kLFOParams; j++)
        {
            int index = baseOffset + j;
            if (m_controls[index])
            {
                m_controls[index]->setVisible(show);
                m_controls[index]->setDirty();
            }
        }
        
        m_curLFO = whichLFO;
        m_LFOButtons[i]->setValue(show? 1.0 : 0);
#if defined(__i386__) && __APPLE__
        // Force screen redraw (ugly hack)
        m_LFOButtons[i]->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
    }
}

void EditorWindow::DrawLFOPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
				  int x, int y, int whichLFO)
{
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);

    int paramOffset = (whichLFO * 8); // 8 parameters per LFO
    
#ifdef DIGITS_PRO
    assert(whichLFO < Tables::kNumLFOs);
#else
    assert(whichLFO == 0);
#endif
    
    // We need to redraw the knobs on LFOs other than the first one, but not
    // the labels
    bool drawLabels;
    if (whichLFO == 0)
    {
        drawLabels = true;
        for (int i=0; i < 3; i++)
        {
            CRect rect = CRect(i*32 + x + kTextboxWidth + 4, y, (i*32)+x+30+kTextboxWidth+4, y+kTextboxHeight);
            const char *title;
            if (i == 0)
                title = "1";
            else if (i == 1)
                title = "2";
            else if (i == 2)
                title = "3";
            else
            {
                title = "";
                assert(false);
            }
            
            CTextButton *button = new CTextButton(rect, this, kGUIParam_LFOButton1+i, title, CTextButton::kOnOffStyle);
            button->setFrameColor(bgColor);
            button->setTextColor(fgColor);
            button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
            button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
            button->setFrameColorHighlighted(fgColor);
            button->setTextColorHighlighted(fgColor);
            destFrame->addView(button);
            m_LFOButtons[i] = button;
        }
    }
    else
    {
        drawLabels = false;
    }

	AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding, y + kSidePadding, VstCore::kLFODelay + paramOffset, drawLabels?"Delay":"");
    AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing, y + kSidePadding, VstCore::kLFORate + paramOffset, drawLabels?"Rate":"");
    AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing*2, y + kSidePadding, VstCore::kLFORepeat + paramOffset, drawLabels?"Repeat":"");
	
	CBitmap* cosNoiseImg = new CBitmap("CosNoiseRockerV.png");
	CRect rRocker(x+kSidePadding+kDialSpacing*3,
                  y+kSidePadding,
				  x+kSidePadding+kDialSpacing*3+cosNoiseImg->getWidth(),
				  y+kSidePadding+31);

    COptionMenu* rocker = new COptionMenu(rRocker, this, VstCore::kLFOType + paramOffset);
    rocker->addEntry("Sine");
    rocker->addEntry("Noise");
    rocker->addEntry("Ramp Up");
    rocker->addEntry("Ramp Dn");
    rocker->setFontColor(bgColorSolid);
    rocker->setBackColor(fgColor);
    rocker->setFrameColor(bgColor);
    destFrame->addView(rocker);
    
	m_controls[VstCore::kLFOType + paramOffset] = rocker;
	cosNoiseImg->forget();
	
    if (drawLabels)
	{
		CRect rLabel = CRect(x+kSidePadding+kDialSpacing*3,
							 y + kSidePadding + dialImg->getHeight(),
							 x+kSidePadding+kDialSpacing*3 + kPulldownWidth,
							 y + kSidePadding + dialImg->getHeight() + kTextboxHeight);
		
		CTextLabel* label = new CTextLabel(rLabel, "Waveshape");
		label->setBackColor(bgColor);
		label->setFontColor(fgColor);
		label->setFrameColor(bgColor);
		label->setHoriAlign(kCenterText);
		destFrame->addView(label);
	}
	
	CBitmap* sliderBGImg = new CBitmap("HSliderBG.png");
	CBitmap* sliderNubImg = new CBitmap("HSliderNub.png");

	int slidersOffY = 60;
	CRect rSlider = CRect(60+x+kSidePadding,
						  y+kSidePadding+slidersOffY,
						  60+x+kSidePadding+sliderBGImg->getWidth(),
						  y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
	CHorizontalSlider* slider = new CHorizontalSlider(rSlider, this, VstCore::kLFOShp + paramOffset,
													  60+x+kSidePadding, 
													  60+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
													  sliderNubImg, sliderBGImg);
	slider->setStyle(kHorizontal);
	slider->setDefaultValue(0);
	destFrame->addView(slider);
	m_controls[VstCore::kLFOShp + paramOffset] = slider;

    CRect rLabel = CRect(x+kSidePadding-8,
                         y+kSidePadding+slidersOffY,
                         x+kSidePadding+60-8,
                         y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
    CTextLabel* label = new CTextLabel(rLabel, "Shaper");

    if (drawLabels)
    {
		
		label->setBackColor(bgColor);
		label->setFontColor(fgColor);
		label->setFrameColor(bgColor);
		label->setHoriAlign(kRightText);
		destFrame->addView(label);

        rLabel.offset(0, 25);
    }
	
	rSlider.offset(0, 25);
    	
	slider = new CHorizontalSlider(rSlider, this, VstCore::kLFOAmp + paramOffset,
													  60+x+kSidePadding, 
													  60+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
													  sliderNubImg, sliderBGImg);
	slider->setStyle(kHorizontal);
	slider->setDefaultValue(0);
	destFrame->addView(slider);
	m_controls[VstCore::kLFOAmp + paramOffset] = slider;


    if (drawLabels)
    {
		label = new CTextLabel(rLabel, "Amplitude");
		label->setBackColor(bgColor);
		label->setFontColor(fgColor);
		label->setFrameColor(bgColor);
		label->setHoriAlign(kRightText);
		destFrame->addView(label);

        rLabel.offset(0, 25);
    }
    
	rSlider.offset(0, 25);
	
	slider = new CHorizontalSlider(rSlider, this, VstCore::kLFOFrq + paramOffset,
								   60+x+kSidePadding, 
								   60+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
								   sliderNubImg, sliderBGImg);
	slider->setStyle(kHorizontal);
	slider->setDefaultValue(0);
	destFrame->addView(slider);
	m_controls[VstCore::kLFOFrq + paramOffset] = slider;
	
    if (drawLabels)
    {
        label = new CTextLabel(rLabel, "Pitch");
        label->setBackColor(bgColor);
        label->setFontColor(fgColor);
        label->setFrameColor(bgColor);
        label->setHoriAlign(kRightText);
        destFrame->addView(label);
	
        rLabel.offset(0, 25);
    }

	rSlider.offset(0, 25);
	
	slider = new CHorizontalSlider(rSlider, this, VstCore::kLFOPWM + paramOffset,
								   60+x+kSidePadding, 
								   60+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
								   sliderNubImg, sliderBGImg);
	slider->setStyle(kHorizontal);
	slider->setDefaultValue(0);
	destFrame->addView(slider);
	m_controls[VstCore::kLFOPWM + paramOffset] = slider;
	
    if (drawLabels)
    {
        label = new CTextLabel(rLabel, "Width");
        label->setBackColor(bgColor);
        label->setFontColor(fgColor);
        label->setFrameColor(bgColor);
        label->setHoriAlign(kRightText);
        destFrame->addView(label);
    }
	
	sliderBGImg->forget();
	sliderNubImg->forget();
	
	
	if (drawLabels)
    {
        rLabel = CRect(x,
                             y,
                             x + kTextboxWidth,
                             y + kTextboxHeight);
        
        label = new CTextLabel(rLabel, "LFO");
        label->setBackColor(fgColor);
        label->setFontColor(bgColorSolid);
        label->setFrameColor(bgColor);
        label->setHoriAlign(kCenterText);
        destFrame->addView(label);
    }
}

void EditorWindow::DrawFilterPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
                                int x, int y)
{
    CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
    CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);
    
    AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing, y + kSidePadding, VstCore::kLockShaperForFilter, "Shp.Lock");
    AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding, y + kSidePadding, VstCore::kFilterReso, "Reso");
    
    CBitmap* cosNoiseImg = new CBitmap("CosNoiseRockerV.png");
    CRect rRocker(x+kSidePadding+kDialSpacing*2,
                  y+kSidePadding,
                  x+kSidePadding+kDialSpacing*2+cosNoiseImg->getWidth(),
                  y+kSidePadding+31);
    
    COptionMenu* rocker = new COptionMenu(rRocker, this, VstCore::kFilterMode);
    rocker->addEntry("24dB");
    rocker->addEntry("18dB");
    rocker->addEntry("12dB");
    rocker->addEntry("6dB");
    rocker->setFontColor(bgColorSolid);
    rocker->setBackColor(fgColor);
    rocker->setFrameColor(bgColor);
    destFrame->addView(rocker);
    
    m_controls[VstCore::kFilterMode] = rocker;
    cosNoiseImg->forget();
    
    CRect rLabel = CRect(x+kSidePadding+kDialSpacing*2,
                         y + kSidePadding + dialImg->getHeight(),
                         x+kSidePadding+kDialSpacing*2 + kTextboxWidth,
                         y + kSidePadding + dialImg->getHeight() + kTextboxHeight);
    
    CTextLabel* label = new CTextLabel(rLabel, "Slope");
    label->setBackColor(bgColor);
    label->setFontColor(fgColor);
    label->setFrameColor(bgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);

    rLabel = CRect(x, y, x+kTextboxWidth, y+kTextboxHeight);
    label = new CTextLabel(rLabel, "Filter");
    label->setBackColor(fgColor);
    label->setFontColor(bgColorSolid);
    label->setFrameColor(fgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);
}

void EditorWindow::DrawMonoPolyPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
                                   int x, int y)
{
    CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
    CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);
    
    AddDial(destFrame, dialImg, dialHandleImg, x + kSidePadding + kDialSpacing, y + kSidePadding, VstCore::kPortaSpeed, "PortaSpd");
    
    CBitmap* cosNoiseImg = new CBitmap("CosNoiseRockerV.png");
    CRect rRocker(x+kSidePadding+kDialSpacing*2,
                  y+kSidePadding,
                  x+kSidePadding+kDialSpacing*2+kPulldownWidth,
                  y+kSidePadding+31);
    
    COptionMenu* rocker = new COptionMenu(rRocker, this, VstCore::kMonoMode);
    rocker->addEntry("Poly");
    rocker->addEntry("PolyGlide");
    rocker->addEntry("Mono");
    rocker->addEntry("MonoGlide");
    rocker->setFontColor(bgColorSolid);
    rocker->setBackColor(fgColor);
    rocker->setFrameColor(bgColor);
    destFrame->addView(rocker);
    
    m_controls[VstCore::kMonoMode] = rocker;
    cosNoiseImg->forget();
    
    CRect rLabel = CRect(x+kSidePadding+kDialSpacing*2,
                         y + kSidePadding + dialImg->getHeight(),
                         (x+kSidePadding+kDialSpacing*2) + kPulldownWidth,
                         y + kSidePadding + dialImg->getHeight() + kTextboxHeight);
    
    CTextLabel* label = new CTextLabel(rLabel, "Voice Mode");
    label->setBackColor(bgColor);
    label->setFontColor(fgColor);
    label->setFrameColor(bgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);
    
    CRect rPitch(x+kSidePadding+kDialSpacing*2+kPulldownWidth + 16,
                  y+kSidePadding,
                  x+kSidePadding+kDialSpacing*2+kPulldownWidth + 16 + 32,
                  y+kSidePadding+31);

    COptionMenu* menu = new COptionMenu(rPitch, this, VstCore::kPitchbendUp);
    for (int i=1; i <= 24; i++)
    {
        char str[3];
        if (i < 10)
        {
            str[0] = '0'+i;
            str[1] = '\0';
        }
        else
        {
            str[0] = '0'+(i/10);
            str[1] = '0'+(i%10);
            str[2] = '\0';
        }
        menu->addEntry(str);
    }

    menu->setFontColor(bgColorSolid);
    menu->setBackColor(fgColor);
    menu->setFrameColor(bgColor);
    destFrame->addView(menu);
    
    m_controls[VstCore::kPitchbendUp] = menu;

    rLabel = CRect(x+kSidePadding+kDialSpacing*2+kPulldownWidth + 16 - 8,
                   y + kSidePadding + dialImg->getHeight(),
                   x+kSidePadding+kDialSpacing*2+kPulldownWidth + 16 + 32 + 8,
                   y + kSidePadding + dialImg->getHeight() + kTextboxHeight);
    
    label = new CTextLabel(rLabel, "Pb.Up");
    label->setBackColor(bgColor);
    label->setFontColor(fgColor);
    label->setFrameColor(bgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);

    rPitch.offset(16+32, 0);

    menu = new COptionMenu(rPitch, this, VstCore::kPitchbendDn);
    for (int i=1; i <= 24; i++)
    {
        char str[3];
        if (i < 10)
        {
            str[0] = '0'+i;
            str[1] = '\0';
        }
        else
        {
            str[0] = '0'+(i/10);
            str[1] = '0'+(i%10);
            str[2] = '\0';
        }
        menu->addEntry(str);
    }
    
    menu->setFontColor(bgColorSolid);
    menu->setBackColor(fgColor);
    menu->setFrameColor(bgColor);
    destFrame->addView(menu);
    
    m_controls[VstCore::kPitchbendDn] = menu;

    rLabel.offset(16+32, 0);
    
    label = new CTextLabel(rLabel, "Pb.Dn");
    label->setBackColor(bgColor);
    label->setFontColor(fgColor);
    label->setFrameColor(bgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);

    rLabel = CRect(x, y, x+kTextboxWidth+12, y+kTextboxHeight);
    label = new CTextLabel(rLabel, "Control");
    label->setBackColor(fgColor);
    label->setFontColor(bgColorSolid);
    label->setFrameColor(fgColor);
    label->setHoriAlign(kCenterText);
    destFrame->addView(label);
}

void EditorWindow::ShowFXPanel(int which)
{
    m_delayButton->setValue(which == 0);
    m_chorusButton->setValue(which == 1);
    m_bitcrushButton->setValue(which == 2);
    m_delayView->setVisible(which == 0);
    m_chorusView->setVisible(which == 1);
    m_bitcrushView->setVisible(which == 2);

#if defined(__i386__) && __APPLE__
    // Force screen redraw (ugly hack)
    m_delayButton->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
    m_chorusButton->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
    m_bitcrushButton->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif

}

void EditorWindow::DrawFXPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
							   int x, int y)
{
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);
    CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);

    static const int frameWidth = 200;
    static const int xOffsetRow1 = (frameWidth - (kDialSpacing*2 + kDialWidth)) / 2;
    static const int xOffsetRow2 = xOffsetRow1 + kDialSpacing*.5;
    
    CRect viewRect = CRect(x, y+kTextboxHeight, x+200, y+126);
    m_delayView = new CViewContainer(viewRect);
    m_chorusView = new CViewContainer(viewRect);
    m_bitcrushView = new CViewContainer(viewRect);
    m_delayView->setBackgroundColor(bgColor);
    m_chorusView->setBackgroundColor(bgColor);
    m_bitcrushView->setBackgroundColor(bgColor);
    
	AddDial(m_delayView, dialImg, dialHandleImg,
                       xOffsetRow1, kSidePadding,
                       VstCore::kDel_L, "LDelay");
	
	AddDial(m_delayView, dialImg, dialHandleImg,
                       xOffsetRow1 + kDialSpacing, kSidePadding,
                       VstCore::kDel_R, "RDelay");
	
	AddDial(m_delayView, dialImg, dialHandleImg,
                       xOffsetRow1 + kDialSpacing*2, kSidePadding,
                       VstCore::kDelWet, "Wet");
	
	AddDial(m_delayView, dialImg, dialHandleImg,
                       xOffsetRow2, kSidePadding + kDialSpacing,
                       VstCore::kDelFB, "FBack");
	
	AddDial(m_delayView, dialImg, dialHandleImg,
                       xOffsetRow2 + kDialSpacing, kSidePadding + kDialSpacing,
                       VstCore::kDelLoss, "Damp");
    

    AddDial(m_chorusView, dialImg, dialHandleImg,
            xOffsetRow1, kSidePadding,
            VstCore::kChrDepth, "Depth");
    
    AddDial(m_chorusView, dialImg, dialHandleImg,
            xOffsetRow1 + kDialSpacing, kSidePadding,
            VstCore::kChrRate, "Rate");
    
    AddDial(m_chorusView, dialImg, dialHandleImg,
            xOffsetRow1 + kDialSpacing*2, kSidePadding,
            VstCore::kChrTone, "Tone");
    
    AddDial(m_chorusView, dialImg, dialHandleImg,
            xOffsetRow1 + kDialSpacing, kSidePadding + kDialSpacing,
            VstCore::kChrDirtyClean, "Dirt");
    
    AddDial(m_chorusView, dialImg, dialHandleImg,
            xOffsetRow1 + kDialSpacing*2, kSidePadding + kDialSpacing,
            VstCore::kChrWetDry, "Wet");
    
    CRect rCheck = CRect(xOffsetRow1 - 20,
                         kSidePadding + kDialSpacing,
                         xOffsetRow1 + kDialSpacing,
                         kSidePadding + kDialSpacing + 32);
    
    CCheckBox *check = new CCheckBox(rCheck, this, VstCore::kChrFlang, "Flanger");
    check->setBoxFillColor(bgColorSolid);
    check->setBoxFrameColor(fgColor);
    check->setCheckMarkColor(fgColor);
    check->setFontColor(fgColor);
    m_chorusView->addView(check);


	
    AddDial(m_bitcrushView, dialImg, dialHandleImg,
           xOffsetRow1, kSidePadding,
           VstCore::kBitDiv, "Divider");

    AddDial(m_bitcrushView, dialImg, dialHandleImg,
           xOffsetRow1 + kDialSpacing, kSidePadding,
           VstCore::kBitBit, "Bits");

    AddDial(m_bitcrushView, dialImg, dialHandleImg,
           xOffsetRow1 + kDialSpacing*2, kSidePadding,
           VstCore::kBitBlend, "Wet");

    destFrame->addView(m_delayView);
    destFrame->addView(m_chorusView);
    destFrame->addView(m_bitcrushView);

    
    
    static const int kBtnSize = 24;
    
    CRect rect = CRect(x + kTextboxWidth + 4, y, x + kTextboxWidth + 4 + kBtnSize, y + kTextboxHeight);
    
    CTextButton *button = new CTextButton(rect, this, kGUIParam_DelButton, "Del", CTextButton::kOnOffStyle);
    button->setFrameColor(bgColorSolid);
    button->setTextColor(fgColor);
    button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
    button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
    button->setFrameColorHighlighted(fgColor);
    button->setTextColorHighlighted(fgColor);
    button->setValue(1.0);
    destFrame->addView(button);
    m_delayButton = button;
    
    rect.offset(kBtnSize+4, 0);
    
    button = new CTextButton(rect, this, kGUIParam_ChrButton, "Chr", CTextButton::kOnOffStyle);
    button->setFrameColor(bgColorSolid);
    button->setTextColor(fgColor);
    button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
    button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
    button->setFrameColorHighlighted(fgColor);
    button->setTextColorHighlighted(fgColor);
    destFrame->addView(button);
    m_chorusButton = button;

    rect.offset(kBtnSize+4, 0);
    
    button = new CTextButton(rect, this, kGUIParam_BitButton, "Bit", CTextButton::kOnOffStyle);
    button->setFrameColor(bgColorSolid);
    button->setTextColor(fgColor);
    button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
    button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
    button->setFrameColorHighlighted(fgColor);
    button->setTextColorHighlighted(fgColor);
    destFrame->addView(button);
    m_bitcrushButton = button;
    
    
	CRect rLabel = CRect(x,
						 y,
						 x + kTextboxWidth,
						 y + kTextboxHeight);
	
	CTextLabel *label = new CTextLabel(rLabel, "FX");
	label->setBackColor(fgColor);
	label->setFontColor(bgColorSolid);
	label->setFrameColor(fgColor);
	label->setHoriAlign(kCenterText);
	destFrame->addView(label);
}

/*
void EditorWindow::DrawFXPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
								 int x, int y)
{
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
	
	static const int kBottomIndent = 16;
	
	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding, y + kSidePadding, 
			VstCore::kDel_L, "LDelay");

	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding + kDialSpacing, y + kSidePadding, 
			VstCore::kDel_R, "RDelay");

	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding + kDialSpacing*2, y + kSidePadding, 
			VstCore::kDelWet, "Wet");

	int bottomRow = dialImg->getHeight() + kTextboxHeight + 10;
	int bottomRow2 = dialImg->getHeight()*2 + kTextboxHeight*2 + 20;
	
	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding, y + kSidePadding + bottomRow, 
			VstCore::kDelFB, "FBack");

	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding + kDialSpacing, y + kSidePadding + bottomRow, 
			VstCore::kDelLoss, "Damp");

	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding + kDialSpacing*2, y + kSidePadding + bottomRow, 
			VstCore::kOrch, "Phat");

	AddDial(destFrame, dialImg, dialHandleImg, 
			x + kSidePadding + kDialSpacing*2, y + kSidePadding + bottomRow2, 
			VstCore::kFM, "FM");
	
	
	CRect rLabel = CRect(x, 
				   y,
				   x + kTextboxWidth,
				   y + kTextboxHeight);
	
	CTextLabel* label = new CTextLabel(rLabel, "FX");
	label->setBackColor(bgColor);
	label->setFontColor(fgColor);
	label->setFrameColor(fgColor);
	label->setHoriAlign(kCenterText);
	destFrame->addView(label);
	
}*/

void EditorWindow::DrawVelocityAftertouchPanel(CFrame* destFrame, CBitmap* dialImg, CBitmap* dialHandleImg,
							   int x, int y, velATMod_t forMode)
{
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
    CColor bgColorSolid = CColor(0xCB, 0xD4, 0xB7);

	CBitmap* sliderBGImg = new CBitmap("HSliderBG.png");
	CBitmap* sliderNubImg = new CBitmap("HSliderNub.png");
	
	int slidersOffY = 0;
	int slidersOffX = 50;
    
    CTextLabel* label;
    int key;
    CRect rLabel = CRect(x+kSidePadding-12,
						 y+kSidePadding+slidersOffY,
						 x+kSidePadding+slidersOffX-8,
						 y+kSidePadding+slidersOffY+sliderBGImg->getHeight());

    if (forMode == kAT) {
        label = new CTextLabel(rLabel, "Shaper");
        m_atLabels[0] = label;
        key = VstCore::kAftertouchShaper;
    }
    else if (forMode == kMod)
    {
        label = new CTextLabel(rLabel, "Shaper");
        m_modLabels[0] = label;
        key = VstCore::kModShaper;
    }
    else {
        label = new CTextLabel(rLabel, "Amplitude");
        m_velLabels[0] = label;
        key = VstCore::kVelVolume;
    }

	CRect rSlider = CRect(slidersOffX+x+kSidePadding,
						  y+kSidePadding+slidersOffY,
						  slidersOffX+x+kSidePadding+sliderBGImg->getWidth(),
						  y+kSidePadding+slidersOffY+sliderBGImg->getHeight());
	CHorizontalSlider* slider = new CHorizontalSlider(rSlider, this, key,
													  slidersOffX+x+kSidePadding, 
													  slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
													  sliderNubImg, sliderBGImg);
	slider->setStyle(kHorizontal);
	slider->setDefaultValue(0);
	destFrame->addView(slider);
	if (forMode == kAT)
        m_controls[VstCore::kAftertouchShaper] = slider;
    else if (forMode == kMod)
        m_controls[VstCore::kModShaper] = slider;
    else
        m_controls[VstCore::kVelVolume] = slider;
		
	label->setBackColor(bgColor);
	label->setFontColor(fgColor);
	label->setFrameColor(bgColor);
	label->setHoriAlign(kRightText);
	destFrame->addView(label);
	
	rSlider.offset(0, 25);
	rLabel.offset(0, 25);
	
    if (forMode == kAT) {
        label = new CTextLabel(rLabel, "LFO1Amp");
        m_atLabels[1] = label;
        key = VstCore::kAftertouchLFO1Amp;
    }
    else if (forMode == kMod)
    {
        label = new CTextLabel(rLabel, "LFO1Amp");
        m_modLabels[1] = label;
        key = VstCore::kModLFO1Amp;
    }
    else {
        label = new CTextLabel(rLabel, "Shaper");
        m_velLabels[1] = label;
        key = VstCore::kVelShaper;
    }
    
	slider = new CHorizontalSlider(rSlider, this, key,
													  slidersOffX+x+kSidePadding, 
													  slidersOffX+x+kSidePadding+sliderBGImg->getWidth()-sliderNubImg->getWidth(),
													  sliderNubImg, sliderBGImg);
	slider->setStyle(kHorizontal);
	slider->setDefaultValue(0);
	destFrame->addView(slider);
    if (forMode == kAT)
        m_controls[VstCore::kAftertouchLFO1Amp] = slider;
    else if (forMode == kMod)
        m_controls[VstCore::kModLFO1Amp] = slider;
    else
        m_controls[VstCore::kVelShaper] = slider;
	
	label->setBackColor(bgColor);
	label->setFontColor(fgColor);
	label->setFrameColor(bgColor);
	label->setHoriAlign(kRightText);
	destFrame->addView(label);
	
	
	sliderNubImg->forget();
	sliderBGImg->forget();
	
    if (forMode == kVel)    // only draw these once
    {
        rLabel = CRect(x,
                             y,
                             x + kTextboxWidth,
                             y + kTextboxHeight);
        
        label = new CTextLabel(rLabel, "Map");
        label->setBackColor(fgColor);
        label->setFontColor(bgColorSolid);
        label->setFrameColor(fgColor);
        label->setHoriAlign(kCenterText);
        destFrame->addView(label);
        
        static const int kBtnSize = 24;
        
        CRect rect = CRect(x + kTextboxWidth + 4, y, x + kTextboxWidth + 4 + kBtnSize, y + kTextboxHeight);
        
        CTextButton *button = new CTextButton(rect, this, kGUIParam_VelButton, "Vel", CTextButton::kOnOffStyle);
        button->setFrameColor(bgColor);
        button->setTextColor(fgColor);
        button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
        button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
        button->setFrameColorHighlighted(fgColor);
        button->setTextColorHighlighted(fgColor);
        destFrame->addView(button);
        m_velButton = button;
        
        rect.offset(kBtnSize+4, 0);

        button = new CTextButton(rect, this, kGUIParam_ModButton, "Mod", CTextButton::kOnOffStyle);
        button->setFrameColor(bgColor);
        button->setTextColor(fgColor);
        button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
        button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
        button->setFrameColorHighlighted(fgColor);
        button->setTextColorHighlighted(fgColor);
        destFrame->addView(button);
        m_modButton = button;
        
        rect.offset(kBtnSize+4, 0);

        
        
        
        
        button = new CTextButton(rect, this, kGUIParam_ATButton, "AT", CTextButton::kOnOffStyle);
        button->setFrameColor(bgColor);
        button->setTextColor(fgColor);
        button->setGradient(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
        button->setGradientHighlighted(CGradient::create(0, 0, bgColorSolid, bgColorSolid));
        button->setFrameColorHighlighted(fgColor);
        button->setTextColorHighlighted(fgColor);
        destFrame->addView(button);
        m_atButton = button;
    }
}

void EditorWindow::DrawLines(CFrame *destFrame)
{
    CColor fgColor = CColor(0x82, 0x83, 0x7F);

    // Border around screen
    
    CRect rect = CRect(0, 0, kEditorWidth, 10);
    CFrame *frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    rect = CRect(0, 0, 10, kEditorHeight);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    rect = CRect(0, kEditorHeight-10, kEditorWidth, kEditorHeight);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    rect = CRect(kEditorWidth-10, 0, kEditorWidth, kEditorHeight);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    
    // Vertical line through Mix and LFO
    rect = CRect(170, 0, 171, 350);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    // Vertical line through AmpEnv and ShpEnv
    rect = CRect(370, 0, 371, 350);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    // Vertical line through Delay and Filter
    rect = CRect(630, 0, 631, 350);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    // Horizontal line through LFO, ShpEnv, Filter
    rect = CRect(170, 150, kEditorWidth, 151);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);
    
    // Horizontal line through Reso
    rect = CRect(0, 250, 170, 251);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);
    
    // Horizontal line through Control and Mono/Poly
    rect = CRect(0, 350, kEditorWidth, 351);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);

    // Vertical line through Mono/Poly
    rect = CRect(210, 350, 211, kEditorHeight);
    frame = new CFrame(rect, this);
    frame->setBackgroundColor(fgColor);
    destFrame->addView(frame);
}



bool EditorWindow::open (void* ptr)
{
	AEffGUIEditor::open(ptr);
    setKnobMode(kLinearMode);
    
	CColor bgColor = CColor(0xCB, 0xD4, 0xB7, 0);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
	
//    CFrame (const CRect& size, VSTGUIEditorInterface* pEditor);
	CRect frameSize (0, 0, kEditorWidth, kEditorHeight);
	CFrame* newFrame = new CFrame(frameSize, this);
    newFrame->open(ptr);

    CBitmap* bgImg = new CBitmap("DigitsBG.png");
    newFrame->setBackground(bgImg);
    bgImg->forget();
	CBitmap* dialImg = new CBitmap("Dial_LCD.png");
	CBitmap* dialHandleImg = new CBitmap("DialHandle_LCD.png");
	
	DrawOscPanel(newFrame, dialImg, dialHandleImg, 10, 10, kOsc1);
	DrawOscPanel(newFrame, dialImg, dialHandleImg, 10, 70, kOsc2);
    DrawPhasePanel(newFrame, dialImg, dialHandleImg, 10, 130);
    DrawOscPanel(newFrame, dialImg, dialHandleImg, 10, 250, kReso);
    
	DrawEnvPanel(newFrame, dialImg, dialHandleImg, 420, 10, true);
    DrawEnvPanel(newFrame, dialImg, dialHandleImg, 420, 150, false);
		
	DrawLFOPanel(newFrame, dialImg, dialHandleImg, 170, 150, 0);
#ifdef DIGITS_PRO
	DrawLFOPanel(newFrame, dialImg, dialHandleImg, 170, 150, 1);
	DrawLFOPanel(newFrame, dialImg, dialHandleImg, 170, 150, 2);
    ShowLFOPanel(0);
#endif

	DrawMixPanel(newFrame, dialImg, dialHandleImg, 170, 10);
	DrawVelocityAftertouchPanel(newFrame, dialImg, dialHandleImg, 10, 350, kVel);
    DrawVelocityAftertouchPanel(newFrame, dialImg, dialHandleImg, 10, 350, kMod);
    DrawVelocityAftertouchPanel(newFrame, dialImg, dialHandleImg, 10, 350, kAT);
    ShowVelATModPanel(kVel);
    DrawMonoPolyPanel(newFrame, dialImg, dialHandleImg, 210, 350);

	DrawFXPanel(newFrame, dialImg, dialHandleImg, 680, 10);
    ShowFXPanel(0);

    DrawFilterPanel(newFrame, dialImg, dialHandleImg, 680, 150);

    DrawPatchPanel(newFrame, 680 + ((200 - 120)/2), 260);

	dialImg->forget();
	dialHandleImg->forget();
	
	CBitmap *logoImg = new CBitmap("Logo.png");
	CRect rLogo = CRect(kEditorWidth - logoImg->getWidth() - 10,
					   kEditorHeight - logoImg->getHeight() - 10,
					   kEditorWidth - 10,
					   kEditorHeight - 10);
	CMovieBitmap *logo = new CMovieBitmap(rLogo, this, 0, logoImg);
	newFrame->addView(logo);
	logoImg->forget();
	
	CRect rText = CRect(8, kEditorHeight - kTextboxHeight - 8,
						300, kEditorHeight - 8);
	m_statusText = new CTextLabel(rText, "");
	m_statusText->setBackColor(bgColor);
	m_statusText->setFrameColor(bgColor);
	m_statusText->setFontColor(fgColor);
	m_statusText->setHoriAlign(kLeftText);
	newFrame->addView(m_statusText);
	
	// vstgui member
	frame = newFrame;
    
//    DrawLines(newFrame);
	
	// Synch up settings
	for (int i=0; i<VstCore::kNumParams; i++)
	{
		setParameter(i, m_me->getParameter(i));
	}

    // Sanity check
    for (int i=0; i < VstCore::kNumParams; i++)
    {
        if (m_controls[i] == 0)
            fprintf(stderr, "Did not set control for %s\n", VstCore::s_paramNames[i]);
    }
    
	return true;
}

void EditorWindow::close ()
{
	CFrame* oldFrame = frame;
	frame = 0;
	oldFrame->forget();
}

void EditorWindow::SavePreset(std::string &patchName)
{
    PatchBankList& pbList = m_me->GetPatchBankList();
    std::string initialDir = pbList.GetUserBankDir().c_str();
    
    // strip '.fxp'
    size_t pos = patchName.find(".fxp");
    if (pos != std::string::npos)
        patchName = patchName.substr(0, pos);
    
    // add FXP to fullpath if the user didn't put it on
    std::string tempStr = patchName;
    std::transform(tempStr.begin(), tempStr.end(),
                   tempStr.begin(), ::tolower);
    if (tempStr.find(".fxp") == std::string::npos)
        patchName.append(".fxp");
    
    m_saveFilenameField->looseFocus();
    m_me->SaveFXP(initialDir + "/" + patchName.c_str());
    // Update to get new file listing, especially if they created
    // a new bank directory
    pbList = m_me->GetPatchBankList();
				
    pbList.GetBankList();
    int newBankIndex = pbList.SetCurBankDir("User");
    pbList.GetPatchList();
    
    // strip '.fxp' again
    tempStr = patchName;
    std::transform(tempStr.begin(), tempStr.end(),
                   tempStr.begin(), ::tolower);
    pos = tempStr.find(".fxp");
    if (pos != std::string::npos)
        patchName = patchName.substr(0, pos);
    int newPatchIndex = pbList.SetCurPatch(patchName);
    // Update to redraw it with the correct bank and patch list
    UpdatePatchPanel();
    // Select correct patch and bank indices
    m_bankMenu->setValue(newBankIndex);
    m_patchMenu->setValue(newPatchIndex);
    m_me->SetProgram(newPatchIndex);
    m_me->SetProgramName(patchName);

}

void EditorWindow::valueChanged(CControl* pControl)
{
	float value;
	int tag = pControl->getTag();
    PatchBankList& pbList = m_me->GetPatchBankList();
	
    if (tag == kBankSelectorKey)
    {
		pbList.SetBankDir(pControl->getValue());
        UpdatePatchPanel(true);
#if defined(__i386__) && __APPLE__
		// This causes the screen to refresh, strangely enough (ugly hack)
		m_patchMenu->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
		m_me->updateDisplay();
        return;
    }
    else if (tag == kPatchSelectorKey)
    {
		int value = pControl->getValue();
		// Last item a special "save" item
		if (value == m_patchMenu->getNbEntries() - 1)
		{
            PatchBankList& pbList = m_me->GetPatchBankList();
            m_saveFilenameField->setText(pbList.GetCurPatchName().c_str());
            m_savePopup->setVisible(true);
		}
		else
		{
			PatchBankList& pbList = m_me->GetPatchBankList();
			pbList.SetCurPatch(pControl->getValue());
            m_me->SetProgram(pControl->getValue());
			m_me->LoadFXP(pbList.GetCurBankDir() + pbList.GetCurPatchName() + ".fxp");
            m_me->SetProgramName(pbList.GetCurPatchName());
			m_statusText->setText(std::string("Loaded " + pbList.GetCurPatchName() + ".fxp").c_str());
			m_statusTimeout = 100;			
#if defined(__i386__) && __APPLE__
			// This causes the screen to refresh, strangely enough (ugly hack)
			m_patchMenu->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
			m_me->updateDisplay();
			
		}
        return;
    }
    else if (tag == VstCore::kO1ShpMod || tag == VstCore::kO2ShpMod || tag == VstCore::kResoWave || tag == VstCore::kFilterMode || tag == VstCore::kMonoMode || tag == VstCore::kLFOType || tag == VstCore::kLFO2Type || tag == VstCore::kLFO3Type)
    {
		value = pControl->getValue() / 4.0f;
    }
    else if (tag == VstCore::kPitchbendDn || tag == VstCore::kPitchbendUp)
    {
        value = pControl->getValue() / 24.0;
    }
	else
    {
		value = pControl->getValue();
    }
    
	effect->setParameterAutomated (tag, value);
}

void EditorWindow::setParameter(VstInt32 index, float value)
{
	if (frame && index < VstCore::kNumParams && m_controls[index])
	{ //1, 3, 5
        if (index == VstCore::kO1ShpMod || index == VstCore::kO2ShpMod || index == VstCore::kResoWave || index == VstCore::kFilterMode || index == VstCore::kMonoMode || index == VstCore::kLFOType || index == VstCore::kLFO2Type || index == VstCore::kLFO3Type)
			value *= 4.0f;
        else if (index == VstCore::kPitchbendUp || index == VstCore::kPitchbendDn)
            value *= 24.0;
//        else if (index == VstCore::kShpMin || index == VstCore::kShpMax)
//            value = pow(value, -2.0);
        
/*        else if (index == VstCore::kLFOShp || index == VstCore::kLFO2Shp || index == VstCore::kLFO3Shp)
        {
            int whichLFO;
            if (index == VstCore::kLFOShp)
                whichLFO = 0;
            else if (index == VstCore::kLFO2Shp)
                whichLFO = 1;
            else if (index == VstCore::kLFO3Shp)
                whichLFO = 2;
            else
            {
                whichLFO = 0;
                assert(false);
            }
            float repeat = m_me->getParameter(VstCore::kLFOShp + kLFOParams*whichLFO);
            int i;
            for (i=0; i < 6; i++)
            {
                if (kLFOTypeTab[i][0] == value && kLFOTypeTab[i][1] == repeat)
                    break;
            }
            assert (i != 6);
            value = i;
        }*/

        
        
        
		static const int kUnitLen = 32;
		m_controls[index]->setValue(value);
		char text[128];
		char units[kUnitLen];
		float textValue = value * 100;
		units[0] = '%';
		units[1] = '\0';
		if (index == VstCore::kGain || index == VstCore::kDelWet || index == VstCore::kDelFB || index == VstCore::kResoVol)
		{
			strncpy(units, " dBFS (amp)", kUnitLen);
			textValue = 20.0f * log10(value);
		}
        else if (index == VstCore::kFilterMode)
        {
            strncpy(units, " dB/oct", kUnitLen);
            textValue = (4-value)*6;
        }
        else if (index == VstCore::kMonoMode)
        {
            if (value == 0)
            {
                textValue = 0;
                strncpy(units, " (poly)", kUnitLen);
            }
            else if (value == 1)
            {
                textValue = 1;
                strncpy(units, " (poly glide)", kUnitLen);
            }
            else if (value == 2)
            {
                textValue = 2;
                strncpy(units, " (mono)", kUnitLen);
            }
            else if (value == 3)
            {
                textValue = 3;
                strncpy(units, " (mono glide)", kUnitLen);
            }
        }
		else if (index == VstCore::kOctave)
		{
			int oct = (int)(value*6.0f) - 1;
			textValue = oct;
			strncpy(units, " octave (osc 1)", kUnitLen);
		}
		else if (index == VstCore::kAmpSus || index == VstCore::kShpSus)
		{
			if (value == 0)
				strncpy(units, " (off)", kUnitLen);
			else if (value == 1.0f)
				strncpy(units, "% (on, no fade)", kUnitLen);
			else
				strncpy(units, "% (on, with fade)", kUnitLen);
		}
		else if (index == VstCore::kO1ShpMod || index == VstCore::kO2ShpMod)
		{
			if (value == 0)
			{
				textValue = 0;
				strncpy(units, " (square)", kUnitLen);
			}
			else if (value == 1)
			{
				textValue = 1;
				strncpy(units, " (saw)", kUnitLen);
			}
			else if (value == 2)
			{
				textValue = 2;
				strncpy(units, " (angry)", kUnitLen);
			}
			else if (value == 3)
			{
				textValue = 3;
				strncpy(units, " (angrier)", kUnitLen);
			}
		}
		else if (index == VstCore::kResoWave)
		{
			if (value == 0)
			{
				textValue = 0;
				strncpy(units, " (saw quarter)", kUnitLen);
			}
			else if (value == 1)
			{
				textValue = 1;
				strncpy(units, " (saw half)", kUnitLen);
			}
			else if (value == 2)
			{
				textValue = 2;
				strncpy(units, " (square quarter)", kUnitLen);
			}
			else if (value == 3)
			{
				textValue = 3;
				strncpy(units, " (square half)", kUnitLen);
			}
		}
        else if (index == VstCore::kPitchbendDn || index == VstCore::kPitchbendUp)
        {
            textValue = floor(value);
            strncpy(units, " (semitones)", kUnitLen);
        }
        
        
		snprintf(text, 128, "%s: %.0f%s", VstCore::s_paramNames[index], textValue, units);
		m_statusText->setText(text);
		m_statusTimeout = 100;
#if defined(__i386__) && __APPLE__
		// Force screen redraw (ugly hack)
		m_patchMenu->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
	}
    else if (index >= kGUIParamsStartAt)
    {
        if (index == kGUIParam_LFOButton1)
            ShowLFOPanel(0);
        else if (index == kGUIParam_LFOButton2)
            ShowLFOPanel(1);
        else if (index == kGUIParam_LFOButton3)
            ShowLFOPanel(2);
        else if (index == kGUIParam_VelButton)
            ShowVelATModPanel(kVel);
        else if (index == kGUIParam_ModButton)
            ShowVelATModPanel(kMod);
        else if (index == kGUIParam_ATButton)
            ShowVelATModPanel(kAT);
        else if (index == kGUIParam_DelButton)
            ShowFXPanel(0);
        else if (index == kGUIParam_ChrButton)
            ShowFXPanel(1);
        else if (index == kGUIParam_BitButton)
            ShowFXPanel(2);
        else if (index == kGUIParam_SaveOK)
        {
            std::string s = m_saveFilenameField->getText().get();
            SavePreset(s);
            m_savePopup->setVisible(false);
            
#if defined(__i386__) && __APPLE__
            // This causes the screen to refresh, strangely enough (ugly hack)
            m_patchMenu->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
            m_me->updateDisplay();
        }
        else if (index == kGUIParam_SaveCancel)
        {
            PatchBankList& pbList = m_me->GetPatchBankList();
            m_patchMenu->setValue(pbList.GetCurPatch());
            m_savePopup->setVisible(false);
            m_saveFilenameField->looseFocus();
        }
    }
}

void EditorWindow::UpdatePatchPanel(bool bankChanged)
{
    if (!m_bankMenu || !m_patchMenu)
        return;

    m_bankMenu->removeAllEntry();
    m_patchMenu->removeAllEntry();

    PatchBankList& pbList = m_me->GetPatchBankList();
    
    PatchBankList::fileList_t bankList = pbList.GetBankList();
    
	if (bankList.size() == 0)
	{
		m_patchMenu->addEntry("<No Patches Found>");
		m_bankMenu->addEntry("<No Banks Found>");
		return;
	}

    // Banks
    for (PatchBankList::fileList_t::iterator it = bankList.begin();
         it != bankList.end();
         it++)
    {
        m_bankMenu->addEntry(it->c_str());
    }

    // Patches
    PatchBankList::fileList_t patchList = pbList.GetPatchList();

    for (PatchBankList::fileList_t::iterator it = patchList.begin();
         it != patchList.end();
         it++)
    {
        m_patchMenu->addEntry(it->c_str());
    }
	m_patchMenu->addEntry("<save patch>");
    
    // Load first patch in the new bank if we're changing
    // to a new bank
    if (bankChanged)
    {
        pbList.SetCurPatch(0);
        m_me->SetProgram(0);
        m_patchMenu->setValue(0);
		// This should fail silently if it's not a real FXP...
		m_me->LoadFXP(pbList.GetCurBankDir() + pbList.GetCurPatchName() + ".fxp");
        m_me->SetProgramName(pbList.GetCurPatchName());
		m_statusText->setText(std::string("Loaded " + pbList.GetCurPatchName() + ".fxp").c_str());
		m_statusTimeout = 100;

#if defined(__i386__) && __APPLE__
		// Force screen redraw (ugly hack)
		m_patchMenu->invalidRect(CRect(0, 0, kEditorWidth, kEditorHeight));
#endif
		
    }

    m_patchMenu->setDirty();
    m_bankMenu->setDirty();
}

void EditorWindow::DrawPatchPanel(CFrame *destFrame, int x, int y)
{
    CRect rect = CRect(CPoint(x, y),
                    CPoint(120, 25));
    m_bankMenu = new COptionMenu(rect, this, kBankSelectorKey);
    
    
    rect.offset(0, 28);
    m_patchMenu = new COptionMenu(rect, this, kPatchSelectorKey);
    
    UpdatePatchPanel();
    m_bankMenu->setValue(m_me->GetPatchBankList().GetCurBank());
    m_patchMenu->setValue(m_me->GetPatchBankList().GetCurPatch());

    CColor bgColor = CColor(0xCB, 0xD4, 0xB7);
	CColor fgColor = CColor(0x82, 0x83, 0x7F);
    m_patchMenu->setFontColor(bgColor);
    m_patchMenu->setBackColor(fgColor);
    m_patchMenu->setFrameColor(bgColor);
    m_bankMenu->setFontColor(bgColor);
    m_bankMenu->setBackColor(fgColor);
    m_bankMenu->setFrameColor(bgColor);
    
    destFrame->addView(m_patchMenu);
    destFrame->addView(m_bankMenu);
    
    
    

    // Create popup for saving
    
    int w = 180;
    x = 640;
    y -= 20;
    rect = CRect(x, y, x+w, y+100);
    m_savePopup = new CViewContainer(rect);
    m_savePopup->setBackgroundColor(fgColor);

    rect = CRect(15, 15, w, 15+10);
    CTextLabel *label = new CTextLabel(rect, "Enter a Name");
    label->setFontColor(bgColor);
    label->setBackColor(fgColor);
    label->setFrameColor(fgColor);
    label->setHoriAlign(kLeftText);
    m_savePopup->addView(label);

    rect = CRect(15, 30, 120, 30+25);
    m_saveFilenameField = new CTextEdit(rect, this, kGUIParam_SaveEnter);
    m_saveFilenameField->setFontColor(fgColor);
    m_saveFilenameField->setBackColor(bgColor);
    m_saveFilenameField->setFrameColor(fgColor);
    m_savePopup->addView(m_saveFilenameField);
    
    rect = CRect(120 + 5, 30, w - 16, 30+25);
    CTextButton *button = new CTextButton(rect, this, kGUIParam_SaveOK, "Save", CTextButton::kKickStyle);
    button->setFrameColor(bgColor);
    button->setTextColor(fgColor);
    button->setGradient(CGradient::create(0, 0, bgColor, bgColor));
    button->setGradientHighlighted(CGradient::create(0, 0, bgColor, bgColor));
    button->setFrameColorHighlighted(fgColor);
    button->setTextColorHighlighted(fgColor);
    m_savePopup->addView(button);

    rect = CRect(120 + 5, 30 + 25 + 5, w - 16, 30+25 + 5 + 25);
    button = new CTextButton(rect, this, kGUIParam_SaveCancel, "Oops", CTextButton::kKickStyle);
    button->setFrameColor(bgColor);
    button->setTextColor(fgColor);
    button->setGradient(CGradient::create(0, 0, bgColor, bgColor));
    button->setGradientHighlighted(CGradient::create(0, 0, bgColor, bgColor));
    button->setFrameColorHighlighted(fgColor);
    button->setTextColorHighlighted(fgColor);
    m_savePopup->addView(button);
    
    m_savePopup->setVisible(false);

    destFrame->addView(m_savePopup);
}

void EditorWindow::idle()
{
	AEffGUIEditor::idle();
	if (!m_statusTimeout)
		m_statusText->setText("");
	else 
		m_statusTimeout--;
}


