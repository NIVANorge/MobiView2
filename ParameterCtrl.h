#ifndef _MobiView2_ParameterCtrl_h_
#define _MobiView2_ParameterCtrl_h_

#include <CtrlLib/CtrlLib.h>

#define LAYOUTFILE <MobiView2/ParameterCtrl.lay>
#include <CtrlCore/lay.h>


class EmptyDisplay : public Upp::Display
{
	public :
		virtual void Paint(Upp::Draw& w, const Upp::Rect& r, const Upp::Value& q, Upp::Color ink, Upp::Color paper, Upp::dword style) const {
			Upp::Display::Paint(w, r, Upp::Null, ink, paper, style);
		}
};

class ParameterCtrl : public WithParameterCtrlLayout<Upp::ParentCtrl>
{
public:
	typedef ParameterCtrl CLASSNAME;
	
	ParameterCtrl();
	
	EmptyDisplay no_display;
	
	bool changed_since_last_save = false;
};

#endif
