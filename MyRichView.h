#ifndef _MobiView_MyLogBox_h_
#define _MobiView_MyLogBox_h_

class MyRichView : public Upp::RichTextView {
public:
	MyRichView() {
		zoomlevel = 7;
		is_log_window = true;
	}
	virtual void Layout() {
		Upp::RichTextView::Layout();
		PageWidth( int(zoomlevel*GetSize().cx) );	// Smaller the total, the bigger the text
		if(is_log_window) ScrollEnd();     //TODO: This is not ideal, but it is better than it always scrolling to top.
	}
	double zoomlevel;
	bool is_log_window;
	
	void append(const Upp::String &to_append) {
		Upp::String data = GetQTF();
		data += to_append;
		SetQTF(data);
	}
	
	virtual void MouseWheel(Upp::Point p, int zdelta, Upp::dword keyflags) {
		if (keyflags == Upp::K_CTRL) {		// Zooms font
			zoomlevel += zdelta/240.;
			if (zoomlevel < 1)
				zoomlevel = 10;
			else if (zoomlevel > 9)
				zoomlevel = 1;
			RefreshLayoutDeep();
		} else				// Scrolls down
			Upp::RichTextView::MouseWheel(p, zdelta, keyflags);
	}
};

#endif
