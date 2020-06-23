//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!

#include <array>
#include "v_font.h"
#include "duke3d.h"
#include "compat.h"
#include "sbar.h"
#include "statusbar.h"
#include "v_draw.h"
#include "texturemanager.h"
BEGIN_DUKE_NS

static FFont* IndexFont;
static FFont* DigiFont;
//==========================================================================
//
// Font init should go elsewhere later.
//
//==========================================================================

void InitFonts()
{
    GlyphSet fontdata;

    // Small font
    for (int i = 0; i < 95; i++)
    {
        auto tile = tileGetTexture(TILE_STARTALPHANUM + i);
        if (tile && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            fontdata.Insert('!' + i, tile);
    }
    SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, -1, false, false, false, &fontdata);
    fontdata.Clear();

    // Big font

    // This font is VERY messy...
    fontdata.Insert('_', tileGetTexture(TILE_BIGALPHANUM - 11));
    fontdata.Insert('-', tileGetTexture(TILE_BIGALPHANUM - 11));
    for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(TILE_BIGALPHANUM - 10 + i));
    for (int i = 0; i < 26; i++) fontdata.Insert('A' + i, tileGetTexture(TILE_BIGALPHANUM + i));
    fontdata.Insert('.', tileGetTexture(TILE_BIGPERIOD));
    fontdata.Insert(',', tileGetTexture(TILE_BIGCOMMA));
    fontdata.Insert('!', tileGetTexture(TILE_BIGX_));
    fontdata.Insert('?', tileGetTexture(TILE_BIGQ));
    fontdata.Insert(';', tileGetTexture(TILE_BIGSEMI));
    fontdata.Insert(':', tileGetTexture(TILE_BIGCOLIN));
    fontdata.Insert('\\', tileGetTexture(TILE_BIGALPHANUM + 68));
    fontdata.Insert('/', tileGetTexture(TILE_BIGALPHANUM + 68));
    fontdata.Insert('%', tileGetTexture(TILE_BIGALPHANUM + 69));
    fontdata.Insert('`', tileGetTexture(TILE_BIGAPPOS));
    fontdata.Insert('"', tileGetTexture(TILE_BIGAPPOS));
    fontdata.Insert('\'', tileGetTexture(TILE_BIGAPPOS));
    BigFont = new ::FFont("BigFont", nullptr, "defbigfont", 0, 0, 0, -1, -1, false, false, false, &fontdata);
    fontdata.Clear();

    // Tiny font
    for (int i = 0; i < 95; i++)
    {
        auto tile = tileGetTexture(TILE_MINIFONT + i);
        if (tile && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            fontdata.Insert('!' + i, tile);
    }
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, -1, false, false, false, &fontdata);
    fontdata.Clear();

    // SBAR index font
    for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(TILE_THREEBYFIVE + i));
    fontdata.Insert(':', tileGetTexture(TILE_THREEBYFIVE + 10));
    fontdata.Insert('/', tileGetTexture(TILE_THREEBYFIVE + 11));
    fontdata.Insert('%', tileGetTexture(TILE_MINIFONT + '%' - '!'));
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    IndexFont = new ::FFont("IndexFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

    fontdata.Clear();

    // digital font
    for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(TILE_DIGITALNUM + i));
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    DigiFont = new ::FFont("DigiFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

}

//==========================================================================
//
// Helpers
//
//==========================================================================

static int32_t G_GetInvAmount(const DukePlayer_t* p)
{
    switch (p->inven_icon)
    {
    case ICON_FIRSTAID:
        return p->inv_amount[GET_FIRSTAID];
    case ICON_STEROIDS:
        return (p->inv_amount[GET_STEROIDS] + 3) >> 2;
    case ICON_HOLODUKE:
        if (RR) return p->inv_amount[GET_HOLODUKE] / 400;
        return (p->inv_amount[GET_HOLODUKE] + 15) / 24;
    case ICON_JETPACK:
        if (RR) return p->inv_amount[GET_JETPACK] / 100;
        return (p->inv_amount[GET_JETPACK] + 15) >> 4;
    case ICON_HEATS:
        return p->inv_amount[GET_HEATS] / 12;
    case ICON_SCUBA:
        return (p->inv_amount[GET_SCUBA] + 63) >> 6;
    case ICON_BOOTS:
        if (RR) return (p->inv_amount[GET_BOOTS] / 10) >> 1;
        return p->inv_amount[GET_BOOTS] >> 1;
    }

    return -1;
}

static int32_t G_GetInvOn(const DukePlayer_t* p)
{
    switch (p->inven_icon)
    {
    case ICON_HOLODUKE:
        return p->holoduke_on;
    case ICON_JETPACK:
        return p->jetpack_on;
    case ICON_HEATS:
        return p->heat_on;
    }

    return 0x80000000;
}

static int32_t GetMoraleOrShield(DukePlayer_t *p, int32_t snum)
{
    // WW2GI
    int lAmount = GetGameVar("PLR_MORALE", -1, p->i, snum);
    if (lAmount == -1) lAmount = p->inv_amount[GET_SHIELD];
    return lAmount;
}

//==========================================================================
//
// very much a dummy to access the methods.
// The goal is to export this to a script.
//
//==========================================================================

class DukeStatusBar : public DBaseStatusBar
{
    DHUDFont numberFont;
    DHUDFont indexFont;
    DHUDFont miniFont;
    DHUDFont digiFont;
    double scale;
    std::array<int, MAX_WEAPONS> ammo_sprites;
    std::array<int, 8> item_icons = { 0, TILE_FIRSTAID_ICON, TILE_STEROIDS_ICON, TILE_HOLODUKE_ICON, TILE_JETPACK_ICON, TILE_HEAT_ICON, TILE_AIRTANK_ICON, TILE_BOOT_ICON };

public:
    DukeStatusBar()
        : numberFont(BigFont, 1, Off, 1, 1),
        indexFont(IndexFont, 4, CellRight, 1, 1),
        miniFont(SmallFont2, 1, Off, 1, 1),
        digiFont(DigiFont, 1 , Off, 1, 1)
    {
        // optionally draw at the top of the screen.
        SetSize(tilesiz[TILE_BOTTOMSTATUSBAR].y);
        drawOffset.Y = hud_position ? -168 : 0;
        scale = (g_gameType & GAMEFLAG_RRALL) ? 0.5 : 1;

        if (!(g_gameType & GAMEFLAG_RRALL))
        {
            ammo_sprites = { -1, TILE_AMMO, TILE_SHOTGUNAMMO, TILE_BATTERYAMMO, TILE_RPGAMMO, TILE_HBOMBAMMO, TILE_CRYSTALAMMO, TILE_DEVISTATORAMMO, TILE_TRIPBOMBSPRITE, TILE_FREEZEAMMO + 1, TILE_HBOMBAMMO, TILE_GROWAMMO/*, FLAMETHROWERAMMO + 1*/ };
        }
        else
        {
            ammo_sprites = { -1, TILE_AMMO, TILE_SHOTGUNAMMO, TILE_BATTERYAMMO, TILE_HBOMBAMMO, TILE_HBOMBAMMO, TILE_RRTILE43, TILE_DEVISTATORAMMO, TILE_TRIPBOMBSPRITE, TILE_GROWSPRITEICON, TILE_HBOMBAMMO, -1,
                TILE_BOWLINGBALLSPRITE, TILE_MOTOAMMO, TILE_BOATAMMO, -1, TILE_RPG2SPRITE };
        }
    }


    //==========================================================================
    //
    // Frag bar - todo
    //
    //==========================================================================
#if 0
    void displayfragbar(void)
    {
        if (ud.statusbarflags & STATUSBAR_NOFRAGBAR)
            return;


        short i, j;

        j = 0;

        for (i = connecthead; i >= 0; i = connectpoint2[i])
            if (i > j) j = i;

        rotatesprite(0, 0, 65600L, 0, TILE_FRAGBAR, 0, 0, 2 + 8 + 16 + 64 + 128, 0, 0, xdim - 1, ydim - 1);
        if (j >= 4) rotatesprite(319, (8) << 16, 65600L, 0, TILE_FRAGBAR, 0, 0, 10 + 16 + 64 + 128, 0, 0, xdim - 1, ydim - 1);
        if (j >= 8) rotatesprite(319, (16) << 16, 65600L, 0, TILE_FRAGBAR, 0, 0, 10 + 16 + 64 + 128, 0, 0, xdim - 1, ydim - 1);
        if (j >= 12) rotatesprite(319, (24) << 16, 65600L, 0, TILE_FRAGBAR, 0, 0, 10 + 16 + 64 + 128, 0, 0, xdim - 1, ydim - 1);

        for (i = connecthead; i >= 0; i = connectpoint2[i])
        {
            minitext(21 + (73 * (i & 3)), 2 + ((i & 28) << 1), &ud.user_name[i][0], sprite[ps[i].i].pal, 2 + 8 + 16 + 128);
            sprintf(tempbuf, "%d", ps[i].frag - ps[i].fraggedself);
            minitext(17 + 50 + (73 * (i & 3)), 2 + ((i & 28) << 1), tempbuf, sprite[ps[i].i].pal, 2 + 8 + 16 + 128);
        }
    }
#endif
    //==========================================================================
    //
    // Common inventory icon code for all styles
    //
    //==========================================================================

    std::pair<const char*, EColorRange> ontext(DukePlayer_t *p)
    {
        std::pair<const char*, EColorRange> retval(nullptr, CR_RED);
        int onstate = G_GetInvOn(p);
        // Texts are intentionally not translated because the font is too small for making localization work and the translated words are too long.
        if ((unsigned)onstate != 0x80000000 && !(g_gameType & (GAMEFLAG_WW2GI|GAMEFLAG_RRALL)))
        {
            retval.second = onstate > 0 ? CR_LIGHTBLUE : CR_RED;
            retval.first = onstate > 0 ? "ON" : "OFF";
        }
        if (p->inven_icon >= ICON_SCUBA)
        {
            retval.second = CR_ORANGE;
            retval.first = "AUTO";
        }
        return retval;
    }

    //==========================================================================
    //
    // draws the inventory selector
    //
    //==========================================================================

    void DrawInventory(const DukePlayer_t* p, double x, double y, int align)
    {
        if (p->invdisptime <= 0)return;

        int n = 0, j = 0;
        if (p->inv_amount[GET_FIRSTAID] > 0) n |= 1, j++;
        if (p->inv_amount[GET_STEROIDS] > 0) n |= 2, j++;
        if (p->inv_amount[GET_HOLODUKE] > 0) n |= 4, j++;
        if (p->inv_amount[GET_JETPACK] > 0) n |= 8, j++;
        if (p->inv_amount[GET_HEATS] > 0) n |= 16, j++;
        if (p->inv_amount[GET_SCUBA] > 0) n |= 32, j++;
        if (p->inv_amount[GET_BOOTS] > 0) n |= 64, j++;

        x -= (j * 11);
        y -= 6;

        ; align |= DI_ITEM_CENTER;
        for(int i = 1; i < 128; i<<=1)
        {
            if (n & i)
            {
                int select = 1 << (p->inven_icon - 1);
                double alpha = select == i ? 1.0 : 0.7;
                switch (i)
                {
                case 1:
                    DrawGraphic(tileGetTexture(TILE_FIRSTAID_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                case 2:
                    DrawGraphic(tileGetTexture(TILE_STEROIDS_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                case 4:
                    DrawGraphic(tileGetTexture(TILE_HOLODUKE_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                case 8:
                    DrawGraphic(tileGetTexture(TILE_JETPACK_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                case 16:
                    DrawGraphic(tileGetTexture(TILE_HEAT_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                case 32:
                    DrawGraphic(tileGetTexture(TILE_AIRTANK_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                case 64:
                    DrawGraphic(tileGetTexture(TILE_BOOT_ICON), x, y, align, alpha, 0, 0, scale, scale);
                    break;
                }
                if (select == i) DrawGraphic(tileGetTexture(TILE_ARROW), x, y, align, alpha, 0, 0, scale, scale);
                x += 22;
            }
        }
    }

    //==========================================================================
    //
    // Fullscreen HUD variant #1
    //
    //==========================================================================

    void FullscreenHUD1(DukePlayer_t* p, int32_t snum)
    {
        //
        // Health
        //
        DrawGraphic(tileGetTexture(TILE_COLA), 2, -2, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, 0.75, 0.75);

        FString format;
        if (!althud_flashing || p->last_extra > (max_player_health >> 2) || ((int32_t)totalclock & 32) || (sprite[p->i].pal == 1 && p->last_extra < 2))
        {
            int s = -8;
            if (althud_flashing && p->last_extra > max_player_health)
                s += (sintable[((int)totalclock << 5) & 2047] >> 10);
            int intens = clamp(255 - 4 * s, 0, 255);
            auto pe = PalEntry(255, intens, intens, intens);
            format.Format("%d", p->last_extra);
            SBar_DrawString(this, &numberFont, format, 40, -BigFont->GetHeight() - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        //
        // Armor
        //
        DrawGraphic(tileGetTexture(TILE_SHIELD), 62, -2, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, 0.75, 0.75);

        format.Format("%d", GetMoraleOrShield(p, snum));
        SBar_DrawString(this, &numberFont, format, 105, -numberFont.mFont->GetHeight() - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

        //
        // Weapon
        //

        int wicon = ammo_sprites[p->curr_weapon];
        if (wicon > 0)
        {
            auto img = tileGetTexture(wicon);
            auto scale = img && img->GetDisplayHeight() >= 50 ? 0.25 : 0.5;
            DrawGraphic(img, -57, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        }

        int weapon = p->curr_weapon;
        if (weapon == HANDREMOTE_WEAPON) weapon = HANDBOMB_WEAPON;

        if (p->curr_weapon != KNEE_WEAPON && (!althud_flashing || (int32_t)totalclock & 32 || p->ammo_amount[weapon] > (max_ammo_amount[weapon] / 10)))
        {
            format.Format("%d", p->ammo_amount[weapon]);
            SBar_DrawString(this, &numberFont, format, -22, -numberFont.mFont->GetHeight() - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        //
        // Selected inventory item
        //

        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = 131;

            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, 1, 1);

            int percentv = G_GetInvAmount(p);
            format.Format("%3d%%", percentv);
            EColorRange color = percentv > 50 ? CR_GREEN : percentv > 25 ? CR_GOLD : CR_RED;
            SBar_DrawString(this, &indexFont, format, x + 35, -indexFont.mFont->GetHeight() - 0.5, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 35, -miniFont.mFont->GetHeight() - 9.5, DI_TEXT_ALIGN_RIGHT, text.second, 1, 0, 0, 1, 1);
        }

        //
        // keys
        //
        if (p->got_access & 1) DrawGraphic(tileGetTexture(TILE_ACCESSCARD), -29, -30, DI_ITEM_CENTER, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        if (p->got_access & 4) DrawGraphic(tileGetTexture(TILE_ACCESSCARD), -24, -28, DI_ITEM_CENTER, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 23));
        if (p->got_access & 2) DrawGraphic(tileGetTexture(TILE_ACCESSCARD), -19, -26, DI_ITEM_CENTER, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 21));
    }


    //==========================================================================
    //
    // Fullscreen HUD variant #2
    //
    //==========================================================================

    void FullscreenHUD2(DukePlayer_t *p)
    {
        //
        // health
        //
        DrawGraphic(tileGetTexture(TILE_HEALTHBOX), 5, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int32_t health = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        FStringf format("%d", health);
        SBar_DrawString(this, &digiFont, format, 19, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // ammo
        //
        DrawGraphic(tileGetTexture(TILE_AMMOBOX), 37, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int wp = (p->curr_weapon == HANDREMOTE_WEAPON) ? HANDBOMB_WEAPON : p->curr_weapon;
        format.Format("%d", p->ammo_amount[wp]);
        SBar_DrawString(this, &digiFont, format, 53, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // inventory
        //
        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = 73;
            DrawGraphic(tileGetTexture(TILE_INVENTORYBOX), 69, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -14, DI_ITEM_LEFT|DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

            int percentv = G_GetInvAmount(p);
            format.Format("%3d%%", percentv);
            EColorRange color = percentv > 50 ? CR_GREEN : percentv > 25 ? CR_GOLD : CR_RED;
            SBar_DrawString(this, &indexFont, format, x + 34, -indexFont.mFont->GetHeight() - 5.5, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 34, -miniFont.mFont->GetHeight() - 14.5, DI_TEXT_ALIGN_RIGHT, text.second, 1, 0, 0, 1, 1);

        }
    }

    //==========================================================================
    //
    // Fullscreen HUD variant #1 for RR
    //
    //==========================================================================

    void FullscreenHUD1RR(DukePlayer_t* p, int32_t snum)
    {

        //
        // Health
        //

        DrawGraphic(tileGetTexture(TILE_SPINNINGNUKEICON+1), 2, -2, DI_ITEM_LEFT_BOTTOM, 1, 0, 0, 10000. / 65536., 10000. / 65536.);

        FString format;
        if (!althud_flashing || p->last_extra > (max_player_health >> 2) || ((int32_t)totalclock & 32) || (sprite[p->i].pal == 1 && p->last_extra < 2))
        {
            int s = -8;
            if (althud_flashing && p->last_extra > max_player_health)
                s += (sintable[((int)totalclock << 5) & 2047] >> 10);
            int intens = clamp(255 - 4 * s, 0, 255);
            auto pe = PalEntry(255, intens, intens, intens);
            format.Format("%d", p->last_extra);
            SBar_DrawString(this, &numberFont, format, 44, -BigFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        //
        // drink
        //
        DrawGraphic(tileGetTexture(TILE_COLA), 70, -2, DI_ITEM_LEFT_BOTTOM, 1, 0, 0, 10000. / 65536., 10000. / 65536.);
        format.Format("%d", p->drink_amt);
        SBar_DrawString(this, &numberFont, format, 98, -BigFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // eat
        //
        DrawGraphic(tileGetTexture(TILE_JETPACK), 122, -2, DI_ITEM_LEFT_BOTTOM, 1, 0, 0, 20000. / 65536., 20000. / 65536.);
        format.Format("%d", p->eat);
        SBar_DrawString(this, &numberFont, format, 175, -BigFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // selected weapon
        //
        int wicon = ammo_sprites[p->curr_weapon];
        if (wicon > 0)
        {
            auto img = tileGetTexture(wicon);
            auto myscale = img && img->GetDisplayHeight() >= 50 ? 0.25 : 0.5;
            DrawGraphic(img, -50, -2, DI_ITEM_RIGHT_BOTTOM, 1, -1, -1, myscale, myscale);
        }

        int weapon = p->curr_weapon;
        if (weapon == HANDREMOTE_WEAPON) weapon = DYNAMITE_WEAPON;

        if (p->curr_weapon != KNEE_WEAPON && p->curr_weapon != SLINGBLADE_WEAPON && (!althud_flashing || (int32_t)totalclock & 32 || p->ammo_amount[weapon] > (max_ammo_amount[weapon] / 10)))
        {
            format.Format("%d", p->ammo_amount[weapon]);
            SBar_DrawString(this, &numberFont, format, -20, -numberFont.mFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        //
        // Selected inventory item
        //

        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = -122;

            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);

            int percentv = G_GetInvAmount(p);
            if (icon <= 2) format.Format("%3d%%", percentv);
            else format.Format("%3d ", percentv);
            SBar_DrawString(this, &miniFont, format, x + 35, -miniFont.mFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 35, -miniFont.mFont->GetHeight() * scale - 9.5, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
        if (p->keys[1]) DrawGraphic(tileGetTexture(TILE_ACCESSCARD), -29, -32, DI_ITEM_BOTTOM, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        if (p->keys[3]) DrawGraphic(tileGetTexture(TILE_ACCESSCARD), -24, -30, DI_ITEM_BOTTOM, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 23));
        if (p->keys[2]) DrawGraphic(tileGetTexture(TILE_ACCESSCARD), -19, -28, DI_ITEM_BOTTOM, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 21));
    }


    //==========================================================================
    //
    // Fullscreen HUD variant #2 for RR
    //
    //==========================================================================

    void FullscreenHUD2RR(DukePlayer_t* p)
    {
        //
        // health
        //
        DrawGraphic(tileGetTexture(TILE_HEALTHBOX), 2, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int32_t health = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        FStringf format("%d", health);
        SBar_DrawString(this, &digiFont, format, 17, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // ammo
        //
        DrawGraphic(tileGetTexture(TILE_AMMOBOX), 41, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int wp = (p->curr_weapon == HANDREMOTE_WEAPON) ? DYNAMITE_WEAPON : p->curr_weapon;
        format.Format("%d", p->ammo_amount[wp]);
        SBar_DrawString(this, &digiFont, format, 57, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // inventory
        //
        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = 81;
            DrawGraphic(tileGetTexture(TILE_INVENTORYBOX), 77, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -14, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

            int percentv = G_GetInvAmount(p);
            if (icon <= 2) format.Format("%3d%%", percentv);
            else format.Format("%3d ", percentv);
            SBar_DrawString(this, &miniFont, format, x + 34, -miniFont.mFont->GetHeight() * scale - 5.5, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
    }

    //==========================================================================
    //
    // Fullscreen HUD drawer
    //
    //==========================================================================

    void DrawHud(int32_t snum, int style)
    {
        auto p = g_player[snum].ps;
        BeginHUD(320, 200, 1.f, true);
        bool rr = !!(g_gameType & GAMEFLAG_RRALL);
        if (style == 1)
        {
            if (!rr)
            {
                DrawInventory(p, 0, -46, DI_SCREEN_CENTER_BOTTOM);
                FullscreenHUD1(p, snum);
            }
            else
            {
                double y = -40;
                if (ud.multimode > 1)
                    y -= 4;
                if (ud.multimode > 4)
                    y -= 4;
                DrawInventory(p, 0, y, DI_SCREEN_CENTER_BOTTOM);
                FullscreenHUD1RR(p, snum);
            }
        }
        else if (style == 2)
        {
            if (!rr)
            {
                DrawInventory(p, (ud.multimode > 1) ? 56 : 65, -28, DI_SCREEN_CENTER_BOTTOM);
                FullscreenHUD2(p);
            }
            else
            {
                DrawInventory(p, 56, -20, DI_SCREEN_CENTER_BOTTOM);
                FullscreenHUD2RR(p);
            }
        }
        else
        {
            DrawInventory(p, 0, rr? -20 : -28, DI_SCREEN_CENTER_BOTTOM);
        }
    }


    //==========================================================================
    //
    // Helper
    //
    //==========================================================================

    PalEntry LightForShade(int shade)
    {
        int ll = clamp((numshades - shade) * 255 / numshades, 0, 255);
        return PalEntry(255, ll, ll, ll);
    }

    //==========================================================================
    //
    // Helper for weapon display
    //
    //==========================================================================

    void DrawWeaponNum(int index, int x, int y, int num1, int num2, int shade, int numdigits)
    {
        /*
        if (VOLUMEONE && (ind > HANDBOMB_WEAPON || ind < 0))
        {
            minitextshade(x + 1, y - 4, "ORDER", 20, 11, 2 + 8 + 16 + ROTATESPRITE_MAX);
            return;
        }
        */
        FString format;

        if (numdigits == 2)
        {
            if (num1 > 99) num1 = 99;
            if (num2 > 99) num2 = 99;
            format.Format("%2d/%d", num1, num2);
        }
        else
        {
            if (num1 > 999) num1 = 999;
            if (num2 > 999) num2 = 999;
            format.Format("%3d/%d", num1, num2);
        }
        y--;
        DrawGraphic(tileGetTexture(TILE_THREEBYFIVE + index), x - 7, y, DI_ITEM_LEFT|DI_ITEM_VCENTER, 1, 0, 0, 1, 1, LightForShade(shade - 10), TRANSLATION(Translation_Remap, 7));
        auto pe = LightForShade(shade);
        DrawGraphic(tileGetTexture(TILE_THREEBYFIVE + 10), x - 3, y, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, 0, 0, 1, 1, pe);
        for (size_t i = 0; i < format.Len(); i++) 
        {
            if (format[i] != ' ')
            {
                char c = format[i] == '/' ? 11 : format[i] - '0';
                DrawGraphic(tileGetTexture(TILE_THREEBYFIVE + c), x + 4 * i, y, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, 0, 0, 1, 1, pe);
            }
        }
    }

    //==========================================================================
    //
    // Weapon display (Duke only)
    //
    //==========================================================================

    void DrawWeaponAmounts(const DukePlayer_t* p, int32_t x, int32_t y)
    {
        int32_t cw = p->curr_weapon;

        auto ShadeForWeapon = [=](int weapon, int optweapon = -1)
        {
            // Headache-inducing math at play here.
            return (((!p->ammo_amount[weapon]) | (!p->gotweapon[weapon])) * 9) + 12 - 18 * ((cw == weapon) || (optweapon != -1 && cw == optweapon));
        };

        DrawWeaponNum(2, x, y, p->ammo_amount[PISTOL_WEAPON], max_ammo_amount[PISTOL_WEAPON], 12 - 20 * (cw == PISTOL_WEAPON), 3);
        DrawWeaponNum(3, x, y + 6, p->ammo_amount[SHOTGUN_WEAPON], max_ammo_amount[SHOTGUN_WEAPON], ShadeForWeapon(SHOTGUN_WEAPON), 3);
        DrawWeaponNum(4, x, y + 12, p->ammo_amount[CHAINGUN_WEAPON], max_ammo_amount[CHAINGUN_WEAPON], ShadeForWeapon(CHAINGUN_WEAPON), 3);
        DrawWeaponNum(5, x + 39, y, p->ammo_amount[RPG_WEAPON], max_ammo_amount[RPG_WEAPON], ShadeForWeapon(RPG_WEAPON), 2);
        DrawWeaponNum(6, x + 39, y + 6, p->ammo_amount[HANDBOMB_WEAPON], max_ammo_amount[HANDBOMB_WEAPON], ShadeForWeapon(HANDBOMB_WEAPON, HANDREMOTE_WEAPON), 2);
        if (p->subweapon & (1 << GROW_WEAPON)) // original code says: if(!p->ammo_amount[SHRINKER_WEAPON] || cw == GROW_WEAPON)
            DrawWeaponNum(7, x + 39, y + 12, p->ammo_amount[GROW_WEAPON], max_ammo_amount[GROW_WEAPON], ShadeForWeapon(GROW_WEAPON), 2);
        else
            DrawWeaponNum(7, x + 39, y + 12, p->ammo_amount[SHRINKER_WEAPON], max_ammo_amount[SHRINKER_WEAPON], ShadeForWeapon(SHRINKER_WEAPON), 2);
        DrawWeaponNum(8, x + 70, y, p->ammo_amount[DEVISTATOR_WEAPON], max_ammo_amount[DEVISTATOR_WEAPON], ShadeForWeapon(DEVISTATOR_WEAPON), 2);
        DrawWeaponNum(9, x + 70, y + 6, p->ammo_amount[TRIPBOMB_WEAPON], max_ammo_amount[TRIPBOMB_WEAPON], ShadeForWeapon(TRIPBOMB_WEAPON), 2);
        DrawWeaponNum(0, x + 70, y + 12, p->ammo_amount[FREEZE_WEAPON], max_ammo_amount[FREEZE_WEAPON], ShadeForWeapon(FREEZE_WEAPON), 2);
    }

    //==========================================================================
    //
    // Status bar drawer
    //
    //==========================================================================

    void Statusbar(int32_t snum)
    {
        auto p = g_player[snum].ps;
        int h = tilesiz[TILE_BOTTOMSTATUSBAR].y;
        int top = 200 - h;
        BeginStatusBar(320, 200, h, true);
        DrawInventory(p, 160, 154, 0);
        DrawGraphic(tileGetTexture(TILE_BOTTOMSTATUSBAR), 0, top, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);

        FString format;

        if (ud.multimode > 1 && (g_gametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
        {
            DrawGraphic(tileGetTexture(TILE_KILLSICON), 228, top + 8, DI_ITEM_OFFSETS, 1, 0, 0, 1, 1);
            format.Format("%d", max(p->frag - p->fraggedself, 0));
            SBar_DrawString(this, &digiFont, format, 287, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }
        else
        {
            auto key = tileGetTexture(TILE_ACCESS_ICON);
            if (p->got_access & 4) DrawGraphic(key, 275, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 23));
            if (p->got_access & 2) DrawGraphic(key, 288, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 21));
            if (p->got_access & 1) DrawGraphic(key, 281, top + 23, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        }
        DrawWeaponAmounts(p, 96, top + 16);

        int num = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        format.Format("%d", num);
        SBar_DrawString(this, &digiFont, format, 32, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        format.Format("%d", GetMoraleOrShield(p, snum));
        SBar_DrawString(this, &digiFont, format, 64, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

        if (p->curr_weapon != KNEE_WEAPON)
        {
            int wep = (p->curr_weapon == HANDREMOTE_WEAPON)? HANDBOMB_WEAPON : p->curr_weapon;
            format.Format("%d", p->ammo_amount[wep]);
            SBar_DrawString(this, &digiFont, format, 208, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        int icon = p->inven_icon;
        if (icon)
        {
            int x = 231;
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, top + 20, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, 1, 1);

            int percentv = G_GetInvAmount(p);
            format.Format("%3d%%", percentv);
            EColorRange color = percentv > 50 ? CR_GREEN : percentv > 25 ? CR_GOLD : CR_RED;
            SBar_DrawString(this, &indexFont, format, x + 34, top + 24, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 34, top + 14, DI_TEXT_ALIGN_RIGHT, text.second, 1, 0, 0, 1, 1);
        }
    }


    //==========================================================================
    //
    // Status bar drawer (RR)
    //
    //==========================================================================

    void DrawWeaponBar(const DukePlayer_t* p, int top)
    {
        double sbscale = 32800. / 65536.;

        DrawGraphic(tileGetTexture(TILE_WEAPONBAR), 0, 158, DI_ITEM_OFFSETS, 1, 0, 0, sbscale, sbscale);

        FString format;
        for (int i = 0; i < 9; i++) 
        {
            if ((g_gameType & GAMEFLAG_RRRA) && i == 4 && p->curr_weapon == CHICKEN_WEAPON)
            {
                DrawGraphic(tileGetTexture(TILE_AMMO_ICON + 10), 18 + i * 32, top - 6, DI_ITEM_OFFSETS, 1, 0, 0, sbscale, sbscale);
                format.Format("%d", p->ammo_amount[CHICKEN_WEAPON]);
            }
            else
            {
                if (p->gotweapon[i+1]) {
                    DrawGraphic(tileGetTexture(TILE_AMMO_ICON + i), 18 + i * 32, top - 6, DI_ITEM_OFFSETS, 1, 0, 0, sbscale, sbscale);
                }
                format.Format("%d", p->ammo_amount[i+1]);
            }
            SBar_DrawString(this, &miniFont, format, 38 + i * 32, 162 - miniFont.mFont->GetHeight() * scale * 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
    }


    //==========================================================================
    //
    // Status bar drawer (RR)
    //
    //==========================================================================

    void StatusbarRR(int32_t snum)
    {
        auto p = g_player[snum].ps;
        double h = tilesiz[TILE_BOTTOMSTATUSBAR].y * scale;
        double top = 200 - h;
        BeginStatusBar(320, 200, h, true);
        DrawInventory(p, 160, 154, 0);

        if (ud.screen_size > 8)
            DrawWeaponBar(p, top);

        DrawGraphic(tileGetTexture(TILE_BOTTOMSTATUSBAR), 0, top, DI_ITEM_LEFT_TOP, 1, -1, -1, scale, scale);

        FString format;

        if (ud.multimode > 1 && (g_gametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
        {
            DrawGraphic(tileGetTexture(TILE_KILLSICON), 228, top + 8, DI_ITEM_OFFSETS, 1, 0, 0, 1, 1);
            format.Format("%d", max(p->frag - p->fraggedself, 0));
            SBar_DrawString(this, &digiFont, format, 287, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
        else
        {
            auto key = tileGetTexture(TILE_ACCESS_ICON);
            if (p->keys[3]) DrawGraphic(key, 140, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale, 0xffffffff, TRANSLATION(Translation_Remap, 23));
            if (p->keys[2]) DrawGraphic(key, 153, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale, 0xffffffff, TRANSLATION(Translation_Remap, 21));
            if (p->keys[1]) DrawGraphic(key, 146, top + 23, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        }

        int num = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        format.Format("%d", num);
        SBar_DrawString(this, &digiFont, format, 64, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        if (p->curr_weapon != KNEE_WEAPON)
        {
            int wep = (p->curr_weapon == HANDREMOTE_WEAPON) ? DYNAMITE_WEAPON : p->curr_weapon;
            format.Format("%d", p->ammo_amount[wep]);
            SBar_DrawString(this, &digiFont, format, 107, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        int icon = p->inven_icon;
        if (icon)
        {
            int x = 183;
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, top + 20, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

            int percentv = G_GetInvAmount(p);
            if (icon <= 2) format.Format("%3d%%", percentv);
            else format.Format("%3d ", percentv);
            SBar_DrawString(this, &miniFont, format, x + 34, top + 24, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

            if (p->inven_icon == ICON_SCUBA || p->inven_icon == ICON_BOOTS) 
                SBar_DrawString(this, &miniFont, "AUTO", x + 34, top + 14, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        p->drunkang = ((p->drink_amt * 8) + 1647) & 2047;
        if (p->drink_amt >= 100)
        {
            p->drink_amt = 100;
            p->drunkang = 400;
        }
        DrawGraphic(tileGetTexture(TILE_GUTMETER), 257, top + 24, DI_ITEM_BOTTOM, 1, -1, -1, scale, scale, 0xffffffff, 0 /*, p->drunkang * 360. / 2048 */ );
        DrawGraphic(tileGetTexture(TILE_GUTMETER), 293, top + 24, DI_ITEM_BOTTOM, 1, -1, -1, scale, scale, 0xffffffff, 0 /*, p->eatang * 360. / 2048 */);

        if (p->drink_amt >= 0 && p->drink_amt <= 30)
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT1), 239, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->drink_amt >= 31 && p->drink_amt <= 65)
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT2), 248, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->drink_amt >= 66 && p->drink_amt <= 87)
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT3), 256, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT4), 265, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }

        if (p->eat >= 0 && p->eat <= 30)
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT1), 276, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->eat >= 31 && p->eat <= 65)
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT2), 285, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->eat >= 66 && p->eat <= 87)
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT3), 294, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else
        {
            DrawGraphic(tileGetTexture(TILE_GUTMETER_LIGHT4), 302, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }

    }

};

void G_DrawStatusBar(int32_t snum)
{
    //if (!RR)
    {
        DukeStatusBar dsb;
        if (ud.screen_size <= 4)
        {
            dsb.DrawHud(snum, ud.screen_size < 4 ? 0 : ud.althud ? 1 : 2);
        }
        else
        {
            if (!RR) dsb.Statusbar(snum);
            else dsb.StatusbarRR(snum);
        }
    }
}

//==========================================================================
//
// Draws the background
// todo: split up to have dedicated functions for both cases.
//
//==========================================================================

void G_DrawBackground(void)
{
    if ((g_player[myconnectindex].ps->gm&MODE_GAME) == 0 && ud.recstat != 2)
    {
        twod->ClearScreen();
        auto tex = tileGetTexture((g_gameType & GAMEFLAG_DEER) ? 7040 : TILE_MENUSCREEN);
        PalEntry color = (g_gameType & GAMEFLAG_DEER) ? 0xffffffff : 0xff808080;
        if (!hud_bgstretch)
            DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, 3, DTA_Color, color, TAG_DONE);
        else
            DrawTexture(twod, tex, 0, 0, DTA_VirtualWidth, twod->GetWidth(), DTA_VirtualHeight, twod->GetHeight(), DTA_KeepRatio, true, DTA_Color, color, TAG_DONE);
        return;
    }

    auto tex = tileGetTexture((g_gameType & GAMEFLAG_RRRA) ? TILE_RRTILE7629 : TILE_BIGHOLE);
    if (tex != nullptr && tex->isValid())
    {
        if (windowxy1.y > 0)
        {
            twod->AddFlatFill(0, 0, twod->GetWidth(), windowxy1.y, tex, false, 1);
        }
        if (windowxy2.y + 1 < twod->GetHeight())
        {
            twod->AddFlatFill(0, windowxy2.y + 1, twod->GetWidth(), twod->GetHeight(), tex, false, 1);
        }
        if (windowxy1.x > 0)
        {
            twod->AddFlatFill(0, windowxy1.y, windowxy1.x, windowxy2.y + 1, tex, false, 1);
        }
        if (windowxy2.x + 1 < twod->GetWidth())
        {
            twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, twod->GetWidth(), windowxy2.y + 1, tex, false, 1);
        }
        auto vb = tileGetTexture(TILE_VIEWBORDER);
        auto ve = tileGetTexture(TILE_VIEWBORDER + 1);
        int x1 = windowxy1.x - 4;
        int y1 = windowxy1.y - 4;
        int x2 = windowxy2.x + 5;
        int y2 = windowxy2.y + 5;
        twod->AddFlatFill(x1, y1, x2, y1 + 4, vb, 5);
        twod->AddFlatFill(x1, y2 - 4, x2, y2, vb, 6);
        twod->AddFlatFill(x1, y1, x1 + 4, y2, vb, 1);
        twod->AddFlatFill(x2 - 4, y1, x2, y2, vb, 3);
        twod->AddFlatFill(x1, y1, x1 + 4, y1 + 4, ve, 1);
        twod->AddFlatFill(x2 - 4, y1, x2, y1 + 4, ve, 3);
        twod->AddFlatFill(x1, y2 - 4, x1 + 4, y2, ve, 2);
        twod->AddFlatFill(x2 - 4, y2 - 4, x2, y2, ve, 4);
    }
    else
    {
        // If we got no frame just clear the screen.
        twod->ClearScreen();
    }
}

END_DUKE_NS
