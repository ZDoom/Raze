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
 