#include "StarOption.h"

#define IMAGECLASS StarImg
#define IMAGEFILE "MobiView2/star.iml"
#include <Draw/iml.h>


void StarOption::Paint(Draw& w) {
	Size sz = GetSize();
	
	Size isz = StarImg::InactiveStar().GetSize();
	Size tsz = GetSmartTextSize(label, font);
	int ix = 0;
	int ty = (sz.cy - tsz.cy) / 2;
	int iy = (tsz.cy - isz.cy) / 2 + ty;

	ix = (sz.cx - isz.cx) / 2;
	iy = (sz.cy - isz.cy) / 2;
	
	int val = Get();
	if(IsNull(val)) val = 0;
	
	if(val == 0)
		w.DrawImage(ix, iy, StarImg::InactiveStar());
	else
		w.DrawImage(ix, iy, StarImg::ActiveStar());
}