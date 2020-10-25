#include "ns.h"
#include "wh.h"
#include "v_font.h"

BEGIN_WH_NS 


//==========================================================================
//
// Sets up the game fonts.
//
//==========================================================================

void InitFonts()
{
	// Remap table for the menu font.
	uint8_t remapbuf[256];
	for (int i = 0; i < 256; i++) remapbuf[i] = i;
	for(int i = 242; i < 252; i++) //yellow to green
		remapbuf[i] = (uint8_t) (368 - i);
	//for(int i = 117; i < 127; i++) //green to yellow
		//remapbuf[i] = (uint8_t) (368 - i);
	lookups.makeTable(20, remapbuf, 0, 0, 0, true);
	
	GlyphSet fontdata;

	// Small font

	for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(THEFONT + 26 + i));
	for (int i = 0; i < 26; i++) fontdata.Insert('A' + i, tileGetTexture(THEFONT + i));

	fontdata.Insert('!', tileGetTexture(1547)); // WH2 is each one less.
	fontdata.Insert('?', tileGetTexture(1548));
	fontdata.Insert('-', tileGetTexture(1549));
	fontdata.Insert('_', tileGetTexture(1549));
	fontdata.Insert(':', tileGetTexture(1550));

	// The texture offsets in this font are useless for font printing. This should only apply to these glyphs, not for international extensions, though.
	GlyphSet::Iterator it(fontdata);
	GlyphSet::Pair* pair;
	while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
	SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 5, false, false, false, &fontdata);
}

 
 END_WH_NS
 