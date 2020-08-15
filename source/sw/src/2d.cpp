//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "screenjob.h"
#include "game.h"
#include "sounds.h"
#include "v_draw.h"
#include "network.h"
#include "menus.h"
#include "gamecontrol.h"
#include "mapinfo.h"
#include "misc.h"
#include "network.h"
#include "pal.h"


BEGIN_SW_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DSWDRealmsScreen : public DScreenJob
{
public:
    DSWDRealmsScreen() : DScreenJob(fadein | fadeout) {}

    int Frame(uint64_t clock, bool skiprequest) override
    {
        const uint64_t duration = 5'000'000'000;
        const auto tex = tileGetTexture(THREED_REALMS_PIC, true);
        const int translation = TRANSLATION(Translation_BasePalettes, DREALMSPAL);

        twod->ClearScreen();
        DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
        return skiprequest ? -1 : clock < duration ? 1 : 0;
    }
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo(const CompletionFunc& completion)
{
    StopSound();
    PlayTheme();

    static const AnimSound logosound[] =
    {
        { 1, DIGI_NOMESSWITHWANG },
        { 5, DIGI_INTRO_SLASH },
        { 15, DIGI_INTRO_WHIRL },
        { -1, -1 }
    };
    static const int logoframetimes[] = { 360, 8, 128 };

    if (!AutoNet && !userConfig.nologo)
	{
		JobDesc jobs[3];
		int job = 0;
		jobs[job++] = { Create<DSWDRealmsScreen>() };
		jobs[job++] = { PlayVideo("sw.anm", logosound, logoframetimes)};
		RunScreenJob(jobs, job, completion, true, true);
	}
	else completion(false);
}

//---------------------------------------------------------------------------
//
// Summary screen
//
//---------------------------------------------------------------------------

static int BonusPunchSound(short)
{
    PlaySound(DIGI_PLAYERYELL3, v3df_none);
    return 0;
}

static int BonusKickSound(short)
{
    PlaySound(DIGI_PLAYERYELL2, v3df_none);
    return 0;
}

static int BonusGrabSound(short)
{
    PlaySound(DIGI_BONUS_GRAB, v3df_none);
    return 0;
}


enum
{
    BONUS_SCREEN_PIC = 5120,
    BONUS_ANIM = 5121,
    BONUS_ANIM_FRAMES = (5159 - 5121),

    BREAK_LIGHT_RATE = 18,

    BONUS_PUNCH = 5121,
    BONUS_KICK = 5136,
    BONUS_GRAB = 5151,
    BONUS_REST = 5121,

    BONUS_TICS = 8,
    BONUS_GRAB_TICS = 20,
    BONUS_REST_TICS = 50,
};

static STATE s_BonusPunch[] =
{
    {BONUS_PUNCH + 0, BONUS_TICS, NULL, &s_BonusPunch[1]},
    {BONUS_PUNCH + 1, BONUS_TICS, NULL, &s_BonusPunch[2]},
    {BONUS_PUNCH + 2, BONUS_TICS, NULL, &s_BonusPunch[3]},
    {BONUS_PUNCH + 2, 0 | SF_QUICK_CALL, BonusPunchSound, &s_BonusPunch[4]},
    {BONUS_PUNCH + 3, BONUS_TICS, NULL, &s_BonusPunch[5]},
    {BONUS_PUNCH + 4, BONUS_TICS, NULL, &s_BonusPunch[6]},
    {BONUS_PUNCH + 5, BONUS_TICS, NULL, &s_BonusPunch[7]},
    {BONUS_PUNCH + 6, BONUS_TICS, NULL, &s_BonusPunch[8]},
    {BONUS_PUNCH + 7, BONUS_TICS, NULL, &s_BonusPunch[9]},
    {BONUS_PUNCH + 8, BONUS_TICS, NULL, &s_BonusPunch[10]},
    {BONUS_PUNCH + 9, BONUS_TICS, NULL, &s_BonusPunch[11]},
    {BONUS_PUNCH + 10, BONUS_TICS, NULL, &s_BonusPunch[12]},
    {BONUS_PUNCH + 11, BONUS_TICS, NULL, &s_BonusPunch[13]},
    {BONUS_PUNCH + 12, BONUS_TICS, NULL, &s_BonusPunch[14]},
    {BONUS_PUNCH + 14, 90,        NULL, &s_BonusPunch[15]},
    {BONUS_PUNCH + 14, BONUS_TICS, NULL, &s_BonusPunch[15]},
};

static STATE s_BonusKick[] =
{
    {BONUS_KICK + 0, BONUS_TICS, NULL, &s_BonusKick[1]},
    {BONUS_KICK + 1, BONUS_TICS, NULL, &s_BonusKick[2]},
    {BONUS_KICK + 2, BONUS_TICS, NULL, &s_BonusKick[3]},
    {BONUS_KICK + 2, 0 | SF_QUICK_CALL, BonusKickSound, &s_BonusKick[4]},
    {BONUS_KICK + 3, BONUS_TICS, NULL, &s_BonusKick[5]},
    {BONUS_KICK + 4, BONUS_TICS, NULL, &s_BonusKick[6]},
    {BONUS_KICK + 5, BONUS_TICS, NULL, &s_BonusKick[7]},
    {BONUS_KICK + 6, BONUS_TICS, NULL, &s_BonusKick[8]},
    {BONUS_KICK + 7, BONUS_TICS, NULL, &s_BonusKick[9]},
    {BONUS_KICK + 8, BONUS_TICS, NULL, &s_BonusKick[10]},
    {BONUS_KICK + 9, BONUS_TICS, NULL, &s_BonusKick[11]},
    {BONUS_KICK + 10, BONUS_TICS, NULL, &s_BonusKick[12]},
    {BONUS_KICK + 11, BONUS_TICS, NULL, &s_BonusKick[13]},
    {BONUS_KICK + 12, BONUS_TICS, NULL, &s_BonusKick[14]},
    {BONUS_KICK + 14, 90,        NULL, &s_BonusKick[15]},
    {BONUS_KICK + 14, BONUS_TICS, NULL, &s_BonusKick[15]},
};

static STATE s_BonusGrab[] =
{
    {BONUS_GRAB + 0, BONUS_GRAB_TICS, NULL, &s_BonusGrab[1]},
    {BONUS_GRAB + 1, BONUS_GRAB_TICS, NULL, &s_BonusGrab[2]},
    {BONUS_GRAB + 2, BONUS_GRAB_TICS, NULL, &s_BonusGrab[3]},
    {BONUS_GRAB + 2, 0 | SF_QUICK_CALL, BonusGrabSound, &s_BonusGrab[4]},
    {BONUS_GRAB + 3, BONUS_GRAB_TICS, NULL, &s_BonusGrab[5]},
    {BONUS_GRAB + 4, BONUS_GRAB_TICS, NULL, &s_BonusGrab[6]},
    {BONUS_GRAB + 5, BONUS_GRAB_TICS, NULL, &s_BonusGrab[7]},
    {BONUS_GRAB + 6, BONUS_GRAB_TICS, NULL, &s_BonusGrab[8]},
    {BONUS_GRAB + 7, BONUS_GRAB_TICS, NULL, &s_BonusGrab[9]},
    {BONUS_GRAB + 8, BONUS_GRAB_TICS, NULL, &s_BonusGrab[10]},
    {BONUS_GRAB + 9, 90,             NULL, &s_BonusGrab[11]},
    {BONUS_GRAB + 9, BONUS_GRAB_TICS, NULL, &s_BonusGrab[11]},
};

static STATE s_BonusRest[] =
{
    {BONUS_REST + 0, BONUS_REST_TICS, NULL, &s_BonusRest[1]},
    {BONUS_REST + 1, BONUS_REST_TICS, NULL, &s_BonusRest[2]},
    {BONUS_REST + 2, BONUS_REST_TICS, NULL, &s_BonusRest[3]},
    {BONUS_REST + 1, BONUS_REST_TICS, NULL, &s_BonusRest[0]},
};

static STATE * s_BonusAnim[] =
{
    s_BonusPunch,
    s_BonusKick,
    s_BonusGrab
};

class DSWLevelSummaryScreen : public DScreenJob
{
    int minutes, seconds, second_tics;
    int nextclock = synctics;
    STATE * State = s_BonusRest;
    int Tics = 0;

    DSWLevelSummaryScreen()
    {
        second_tics = (PlayClock / 120);
        minutes = (second_tics / 60);
        seconds = (second_tics % 60);
    }

    static void gNextState(STATE** State)
    {
        // Transition to the next state
        *State = (*State)->NextState;

        if (TEST((*State)->Tics, SF_QUICK_CALL))
        {
            (*(*State)->Animator)(0);
            *State = (*State)->NextState;
        }
    }


    // Generic state control
    static void gStateControl(STATE** State, int* tics)
    {
        *tics += synctics;

        // Skip states if too much time has passed
        while (*tics >= (*State)->Tics)
        {
            // Set Tics
            *tics -= (*State)->Tics;
            gNextState(State);
        }

        // Call the correct animator
        if ((*State)->Animator)
            (*(*State)->Animator)(0);
    }

    int Frame(uint64_t clock, bool skiprequest)
    {
        twod->ClearScreen();
        int totalclock = int(clock * 120 / 1'000'000'000);

        while (totalclock > nextclock)
        {
            nextclock += synctics;

            if (State >= s_BonusRest && State < &s_BonusRest[countof(s_BonusRest)])
            {
                State = s_BonusAnim[STD_RANDOM_RANGE(countof(s_BonusAnim))];
                Tics = 0;
            }
            gStateControl(&State, &Tics);
        }
        twod->ClearScreen();
        DrawTexture(twod, tileGetTexture(BONUS_SCREEN_PIC, true), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
        MNU_DrawString(160, 20, currentLevel->DisplayName(), 1, 19, 0);
        MNU_DrawString(170, 30, GStrings("COMPLETED"), 1, 19, 0);

        DrawTexture(twod, tileGetTexture(State->Pic), 158, 86, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
            DTA_TopLeft, true, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

        auto BONUS_LINE = [](int i) { return (50 + ((i) * 20)); };

        int line = 0;
        FString ds;
        ds.Format("%s  %2d : %02d", GStrings("TXT_YOURTIME"), minutes, seconds);
        MNU_DrawString(60, BONUS_LINE(line++), ds, 1, 16);

        if (currentLevel->designerTime > 0)
        {
            ds.Format("%s  %d:%02d", GStrings("TXT_3DRTIME"), currentLevel->designerTime / 60, currentLevel->designerTime % 60);
            MNU_DrawString(40, BONUS_LINE(line++), ds, 1, 16);
        }

        if (currentLevel->parTime > 0)
        {
            ds.Format("%s  %d:%02d", GStrings("TXT_PARTIME"), currentLevel->parTime / 60, currentLevel->parTime % 60);
            MNU_DrawString(40, BONUS_LINE(line++), ds, 1, 16);
        }

        // always read secrets and kills from the first player
        ds.Format("%s:  %d / %d", GStrings("SECRETS"), Player->SecretsFound, LevelSecrets);
        MNU_DrawString(60, BONUS_LINE(line++), ds, 1, 16);

        ds.Format("%s:  %d / %d", GStrings("KILLS"), Player->Kills, TotalKillable);
        MNU_DrawString(60, BONUS_LINE(line), ds, 1, 16);

        MNU_DrawString(160, 185, GStrings("PRESSKEY"), 1, 19, 0);

        int ret = (State == State->NextState)? 0 : skiprequest ? -1 : 1;
        if (ret != 1) StopSound();
        return ret;
    }

};

//---------------------------------------------------------------------------
//
// Deathmatch summary screen
//
//---------------------------------------------------------------------------

static constexpr int SM_SIZ(int num) { return (num * 4); }

enum
{
    STAT_SCREEN_PIC = 5114,

    STAT_START_X = 20,
    STAT_START_Y = 85,
    STAT_OFF_Y = 9,
    STAT_HEADER_Y = 14,
    STAT_TABLE_X = (STAT_START_X + SM_SIZ(15)),
    STAT_TABLE_XOFF = SM_SIZ(6)
};


class DSWMultiSummaryScreen : public DScreenJob
{
    short death_total[MAX_SW_PLAYERS_REG]{};
    short kills[MAX_SW_PLAYERS_REG]{};

    DSWMultiSummaryScreen()
    {
    }

    int Frame(uint64_t clock, bool skiprequest)
    {
        if (clock == 0) PlaySong(nullptr, ThemeSongs[1], ThemeTrack[1]);

        twod->ClearScreen();
        DrawTexture(twod, tileGetTexture(STAT_SCREEN_PIC, true), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
        MNU_DrawString(160, 68, GStrings("MULTIPLAYER TOTALS"), 0, 0);
        MNU_DrawString(160, 189, GStrings("PRESSKEY"), 0, 0, 0);

        int x = STAT_START_X;
        int y = STAT_START_Y;

        // Hm.... how to translate this without messing up the formatting?
        DisplayMiniBarSmString(x, y, 0, "  NAME         1     2     3     4     5     6     7    8     KILLS");
        int rows = OrigCommPlayers;
        int cols = OrigCommPlayers;

        y += STAT_HEADER_Y;

        FString ds;
        for (int i = 0; i < rows; i++)
        {
            x = STAT_START_X;
            auto pp = Player + i;

            ds.Format("%d", i + 1);
            DisplayMiniBarSmString(x, y, 0, ds);

            ds.Format("  %-13s", pp->PlayerName);
            DisplayMiniBarSmString(x, y, User[pp->PlayerSprite]->spal, ds);

            x = STAT_TABLE_X;
            for (int j = 0; j < cols; j++)
            {
                int pal = 0;
                death_total[j] += pp->KilledPlayer[j];

                if (i == j)
                {
                    // don't add kill for self or team player
                    pal = PALETTE_PLAYER0 + 4;
                    kills[i] -= pp->KilledPlayer[j];  // subtract self kills
                }
                else if (gNet.TeamPlay)
                {
                    if (User[pp->PlayerSprite]->spal == User[Player[j].PlayerSprite]->spal)
                    {
                        // don't add kill for self or team player
                        pal = PALETTE_PLAYER0 + 4;
                        kills[i] -= pp->KilledPlayer[j];  // subtract self kills
                    }
                    else
                        kills[i] += pp->KilledPlayer[j];  // kills added here
                }
                else
                {
                    kills[i] += pp->KilledPlayer[j];  // kills added here
                }

                ds.Format("%d", pp->KilledPlayer[j]);
                DisplayMiniBarSmString(x, y, pal, ds);
                x += STAT_TABLE_XOFF;
            }

            y += STAT_OFF_Y;
        }


        // Deaths

        x = STAT_START_X;
        y += STAT_OFF_Y;

        ds.Format("   %s", GStrings("DEATHS"));
        DisplayMiniBarSmString(x, y, 0, ds);
        x = STAT_TABLE_X;

        for (int j = 0; j < cols; j++)
        {
            ds.Format("%d", death_total[j]);
            DisplayMiniBarSmString(x, y, 0, ds);
            x += STAT_TABLE_XOFF;
        }

        x = STAT_START_X;
        y += STAT_OFF_Y;

        // Kills
        x = STAT_TABLE_X + SM_SIZ(50);
        y = STAT_START_Y + STAT_HEADER_Y;

        for (int i = 0; i < rows; i++)
        {
            auto pp = Player + i;

            ds.Format("%d", kills[i]); //pp->Kills);
            DisplayMiniBarSmString(x, y, 0, ds);

            y += STAT_OFF_Y;
        }
        if (skiprequest) StopSound();
        return skiprequest ? -1 : 1;
    }
};

END_SW_NS
