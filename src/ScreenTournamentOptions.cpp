#include "global.h"
#include "RageLog.h"
#include "TournamentManager.h"
#include "ProfileManager.h"
#include "ScreenManager.h"
#include "ScreenTextEntry.h"
#include "ScreenMiniMenu.h"
#include "ScreenTournamentOptions.h"
#include "Profile.h"
#include "PlayerNumber.h"
#include <cstdio>

#define PREV_SCREEN		THEME->GetMetric( m_sName, "PrevScreen" )
#define NEXT_SCREEN		THEME->GetMetric( m_sName, "NextScreen" )

REGISTER_SCREEN_CLASS( ScreenTournamentOptions );

AutoScreenMessage( SM_BackFromRegisterMenu );
AutoScreenMessage( SM_BackFromModifyMenu );

AutoScreenMessage( SM_LoadFromUSB );
AutoScreenMessage( SM_SetDisplayName );
AutoScreenMessage( SM_SetScoreName );
AutoScreenMessage( SM_SetSeed );
AutoScreenMessage( SM_RegisterPlayer );
AutoScreenMessage( SM_DeletePlayer );

// globals to store current player data
CString g_sCurPlayerName = "";
CString g_sCurScoreName = "";
unsigned g_iCurSeedIndex = 0;
bool g_bRegistered = false;
int g_iCurPlayerIndex = -1;

Profile *g_pCurProfile;
Competitor *g_pCurCompetitor;

enum TournamentOptionsLine
{
	PO_ADD_PLAYER,
	PO_MODIFY_PLAYER,
	NUM_TOURNAMENT_OPTIONS_LINES,
};	

OptionRowDefinition g_TournamentOptionsLines[NUM_TOURNAMENT_OPTIONS_LINES] =
{
	OptionRowDefinition( "AddPlayer", true, "Press START" ),
	OptionRowDefinition( "ModifyPlayer", true ),
};

enum RegisterMenuChoice
{
	load_from_usb,
	set_player_name,
	set_highscore_name,
	set_seed_number,
	exit_and_register,
	exit_and_cancel,
	exit_and_delete,
	NUM_REGISTER_MENU_CHOICES
};

/* Shared row definitions.
 * UGLY: MenuRow assumes Edit mode, but this isn't. We'll fix this up sometime... */
MenuRow g_RegisterMenuRows[NUM_REGISTER_MENU_CHOICES] =
{
	MenuRow( load_from_usb,		"Load from USB",	true, 	EDIT_MODE_PRACTICE, 0 ),
	MenuRow( set_player_name,	"Set display name",	true, 	EDIT_MODE_PRACTICE, 0, g_sCurPlayerName ),
	MenuRow( set_highscore_name,	"Set highscore name",	true, 	EDIT_MODE_PRACTICE, 0, g_sCurScoreName ),
	MenuRow( set_seed_number,	"Set seed (optional)",	true, 	EDIT_MODE_PRACTICE, 0, ssprintf("%i", g_iCurSeedIndex) ),
	MenuRow( exit_and_register,	"Finish",		false,	EDIT_MODE_PRACTICE, 0 ),
	MenuRow( exit_and_cancel,	"Cancel",		true, 	EDIT_MODE_PRACTICE, 0 ),
	MenuRow( exit_and_delete,	"Delete",		true, 	EDIT_MODE_PRACTICE, 0 ),
};

// used to load a new registrant
static Menu g_RegisterMenu
(
	"ScreenMiniMenuRegisterMenu",
	g_RegisterMenuRows[load_from_usb],
	g_RegisterMenuRows[set_player_name],
	g_RegisterMenuRows[set_highscore_name],
	g_RegisterMenuRows[set_seed_number],
	g_RegisterMenuRows[exit_and_register],
	g_RegisterMenuRows[exit_and_cancel]
);

// used to modify or cancel - has a different setup
static Menu g_RegisterModifyMenu
(
	"ScreenMiniMenuRegisterModifyMenu",
	g_RegisterMenuRows[set_player_name],
	g_RegisterMenuRows[set_highscore_name],
	g_RegisterMenuRows[set_seed_number],
	g_RegisterMenuRows[exit_and_register],
	g_RegisterMenuRows[exit_and_cancel],
	g_RegisterMenuRows[exit_and_delete]
);

// helper functions for menu rendering
void RefreshRegisterMenuRows()
{
	// set the values for each row accordingly
	g_RegisterMenuRows[set_player_name].choices[0] = g_sCurPlayerName;
	g_RegisterMenuRows[set_highscore_name].choices[0] = g_sCurScoreName;
	g_RegisterMenuRows[set_seed_number].choices[0] = ssprintf( "%i", g_iCurSeedIndex );

	// if the names are filled in, enable registration
	g_RegisterMenuRows[exit_and_register].bEnabled = (!g_sCurPlayerName.empty() && !g_sCurScoreName.empty());

	for( RegisterMenuChoice r = set_player_name; r <= exit_and_register; enum_add<RegisterMenuChoice>(r, 1) )
	{
		g_RegisterMenu.rows[r] = g_RegisterMenuRows[r];
		g_RegisterModifyMenu.rows[r] = g_RegisterMenuRows[r];
	}
}

void DisplayRegisterMiniMenu( bool bModifyMenu )
{
	RefreshRegisterMenuRows();

	if( bModifyMenu )
		SCREENMAN->MiniMenu( &g_RegisterModifyMenu, SM_BackFromModifyMenu );
	else
		SCREENMAN->MiniMenu( &g_RegisterMenu, SM_BackFromRegisterMenu );
}

void LoadPlayerDataFromCompetitor()
{
	LOG->Debug( "g_pCurCompetitor: %s, %s, %i", g_pCurCompetitor->sDisplayName.c_str(), g_pCurCompetitor->sHighScoreName.c_str(), g_pCurCompetitor->iSeedIndex );
	g_sCurPlayerName = g_pCurCompetitor->sDisplayName;
	g_sCurScoreName = g_pCurCompetitor->sHighScoreName;
	g_iCurSeedIndex = g_pCurCompetitor->iSeedIndex;
	g_bRegistered = true;
	LOG->Debug( "Player data: %s, %s, %i", g_sCurPlayerName.c_str(), g_sCurScoreName.c_str(), g_iCurSeedIndex );

	if( g_iCurPlayerIndex == -1 )
		g_iCurPlayerIndex = TOURNAMENT->FindCompetitorIndex( g_pCurCompetitor );
}

void SavePlayerDataToCompetitor()
{
	g_pCurCompetitor->sDisplayName = g_sCurPlayerName;
	g_pCurCompetitor->sHighScoreName = g_sCurScoreName;
	g_pCurCompetitor->iSeedIndex = g_iCurSeedIndex;
}

// this requires a void* argument due to ScreenPrompt
void DeleteCompetitor( void *pThrowAway )
{
	// debugging
	if( !TOURNAMENT->DeleteCompetitor(g_pCurCompetitor) )
		SCREENMAN->SystemMessage( "Unable to delete competitor." );
}

// this requires a void* argument due to ScreenPrompt
void LoadPlayerDataFromProfile( void *pThrowAway )
{
	g_sCurPlayerName = g_pCurProfile->GetDisplayName();
	g_sCurScoreName = g_pCurProfile->m_sLastUsedHighScoreName;
}

void ResetPlayerData()
{
	g_sCurPlayerName = "";
	g_sCurScoreName = "";
	g_iCurSeedIndex = 0;
	g_bRegistered = false;
	g_iCurPlayerIndex = -1;
}

ScreenTournamentOptions::ScreenTournamentOptions( CString sClassName ) : ScreenOptions( sClassName )
{
	LOG->Debug( "ScreenTournamentOptions::ScreenTournamentOptions()" );

	TOURNAMENT->StartTournament();
}

ScreenTournamentOptions::~ScreenTournamentOptions()
{
	LOG->Debug( "ScreenTournamentOptions::~ScreenTournamentOptions()" );
	SAFE_DELETE( g_pCurProfile );
}

void ScreenTournamentOptions::Init()
{
	ScreenOptions::Init();

	g_pCurProfile = new Profile;

	FOREACH_ENUM( TournamentOptionsLine, NUM_TOURNAMENT_OPTIONS_LINES, LINE )
		g_TournamentOptionsLines[LINE].choices.clear();

	// enable all lines for all players
	for( unsigned i = 0; i < NUM_TOURNAMENT_OPTIONS_LINES; i++ )
		FOREACH_PlayerNumber( pn )
			g_TournamentOptionsLines[i].m_vEnabledForPlayers.insert( pn );

	g_TournamentOptionsLines[PO_ADD_PLAYER].choices.push_back( "Press START" );


	// populate player data, if any
	if( TOURNAMENT->GetNumCompetitors() > 0 )
		TOURNAMENT->GetCompetitorNames( g_TournamentOptionsLines[PO_MODIFY_PLAYER].choices, true );
	else
		g_TournamentOptionsLines[PO_MODIFY_PLAYER].choices.push_back( "-NO ENTRIES-" );

	
	vector<OptionRowDefinition> vDefs( &g_TournamentOptionsLines[0], &g_TournamentOptionsLines[ARRAYSIZE(g_TournamentOptionsLines)] );
	vector<OptionRowHandler*> vHands( vDefs.size(), NULL );
	InitMenu( INPUTMODE_SHARE_CURSOR, vDefs, vHands );

	// init the MiniMenu data
	RefreshRegisterMenuRows();
}

void ScreenTournamentOptions::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	CHECKPOINT_M( "Input start" );
	ScreenOptions::Input( DeviceI, type, GameI, MenuI, StyleI );
	CHECKPOINT_M( "Input end" );
}

void ScreenTournamentOptions::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_BackFromRegisterMenu || SM == SM_BackFromModifyMenu )
	{
		switch( ScreenMiniMenu::s_iLastRowCode )
		{
		case load_from_usb:
			{
				// we shouldn't be here
				ASSERT( !g_bRegistered );

				g_pCurProfile->InitEditableData(); // clobber any previous data

				CString sMessage;
				PromptType pt = PROMPT_OK;

				FOREACH_PlayerNumber( pn )
				{
					Profile::LoadResult lr = PROFILEMAN->LoadEditableDataFromMemoryCard(pn, g_pCurProfile);

					// no profile here - keep looking
					if( lr == Profile::failed_no_profile )
						continue;
					else if( lr == Profile::failed_tampered )
						sMessage = ssprintf( "Could not load data for Player %i.", pn+1 );
					else if( lr == Profile::success )
					{
						pt = PROMPT_YES_NO;
						sMessage = ssprintf( "Player name: %s\nScore name: %s\n\nIs this correct?", 
							g_pCurProfile->GetDisplayName().c_str(), g_pCurProfile->m_sLastUsedHighScoreName.c_str() );
					}

					// if it failed with a profile, or succeeded, end here
					break;
				}

				if( sMessage.empty() )
					sMessage = "No cards available to load data from!";

				SCREENMAN->Prompt( SM_LoadFromUSB, sMessage, pt, ANSWER_NO, &LoadPlayerDataFromProfile );
			}
			break;
		case set_player_name:
			SCREENMAN->TextEntry( SM_SetDisplayName, ssprintf( "Player %u's name:", TOURNAMENT->GetNumCompetitors()+1), g_sCurPlayerName, 12 );
			break;
		case set_highscore_name:
			SCREENMAN->TextEntry( SM_SetScoreName, ssprintf( "Highscore name for %s:", g_sCurPlayerName.c_str()),  g_sCurScoreName, 4 );
			break;
		case set_seed_number:
			SCREENMAN->TextEntry( SM_SetSeed, ssprintf( "Seed index for %s:", g_sCurPlayerName.c_str()), "", 3 );
			break;
		case exit_and_register:
			{
				// handler for a new registration
				if( SM == SM_BackFromRegisterMenu )
				{
					CString sError;
					bool bRegistered = TOURNAMENT->RegisterCompetitor( g_sCurPlayerName, g_sCurScoreName, g_iCurSeedIndex, sError );

					if( bRegistered )
					{
						SCREENMAN->SystemMessage( ssprintf("Registered \"%s\" as Player %i",
							g_sCurPlayerName.c_str(), TOURNAMENT->GetNumCompetitors()) );

						// we're registered - reset and prepare for new data	
						ResetPlayerData(); 
						ReloadScreen();
					}
					else
					{
						SCREENMAN->SystemMessage( ssprintf( "Registration error: %s", sError.c_str()) );
					}
				}
				else if( SM == SM_BackFromModifyMenu )
				{
					ASSERT( g_pCurCompetitor );
					SavePlayerDataToCompetitor();
					ResetPlayerData();

					// reload any new data we've been given
					ReloadScreen();
				}
			}
			break;
		case exit_and_delete:
			{
				CString sMessage = ssprintf( "Are you sure you want to delete\nPlayer %i (\"%s\")?", 
					g_iCurPlayerIndex+1, g_pCurCompetitor->sDisplayName.c_str() );
				SCREENMAN->Prompt( SM_DeletePlayer, sMessage, PROMPT_YES_NO, ANSWER_NO, &DeleteCompetitor );
			}
			break;
		case exit_and_cancel:
			// we shouldn't be here
			ResetPlayerData();
			break;
		}
	}
	else if( SM == SM_SetDisplayName )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
			g_sCurPlayerName = ScreenTextEntry::s_sLastAnswer;
		DisplayRegisterMiniMenu( g_bRegistered ); // return to the menu
	}
	else if( SM == SM_SetScoreName )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
			g_sCurScoreName = ScreenTextEntry::s_sLastAnswer;
		g_sCurScoreName.MakeUpper();
		DisplayRegisterMiniMenu( g_bRegistered ); // return to the menu
	}
	else if( SM == SM_SetSeed )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
			g_iCurSeedIndex = (int) strtof( ScreenTextEntry::s_sLastAnswer, NULL );
		DisplayRegisterMiniMenu( g_bRegistered ); // return to the menu
	}
	else if( SM == SM_LoadFromUSB )
	{
		DisplayRegisterMiniMenu( false );
	}
	else if( SM == SM_DeletePlayer )
	{
		ReloadScreen();
	}
	else if( SM == SM_GoToPrevScreen )
	{
		SCREENMAN->SetNewScreen( PREV_SCREEN );
	}
	else if( SM == SM_GoToNextScreen )
	{
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
	}
}

void ScreenTournamentOptions::MenuStart( PlayerNumber pn, const InputEventType type )
{
	if( type != IET_FIRST_PRESS )
		return;

	switch( GetCurrentRow() )
	{
	case PO_ADD_PLAYER:
		DisplayRegisterMiniMenu( false );
		break;
	case PO_MODIFY_PLAYER:
		{
			// ignore if we don't have any actual entries yet
			if( TOURNAMENT->GetNumCompetitors() == 0 )
				break;

			int iIndex = m_Rows[PO_MODIFY_PLAYER]->GetOneSharedSelection( false );
			g_pCurCompetitor = TOURNAMENT->GetCompetitorByIndex( iIndex );
			g_iCurPlayerIndex = iIndex;

			LOG->Debug( "iIndex: %i, g_pCurCompetitor's name: %s", iIndex, g_pCurCompetitor->sDisplayName.c_str() );
			LoadPlayerDataFromCompetitor();

			DisplayRegisterMiniMenu( true );
		}
		break;
	default:
		ScreenOptions::MenuStart( pn, type );
	}
}

void ScreenTournamentOptions::MenuBack( PlayerNumber pn, const InputEventType type )
{
	if( !IsTransitioning() )
	{
		this->PlayCommand( "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );
	}
}

void ScreenTournamentOptions::ImportOptions( int row, const vector<PlayerNumber> &vpns )
{
	LOG->Debug( "ScreenTournamentOptions::ImportOptions( %i, vpns )", row );
}

void ScreenTournamentOptions::ExportOptions( int row, const vector<PlayerNumber> &vpns )
{
	LOG->Debug( "ScreenTournamentOptions::ExportOptions( %i, vpns )", row );
}

void ScreenTournamentOptions::GoToPrevScreen()
{
	SCREENMAN->SetNewScreen( PREV_SCREEN );	
}

void ScreenTournamentOptions::GoToNextScreen()
{
	SCREENMAN->SetNewScreen( NEXT_SCREEN );
}

void ScreenTournamentOptions::ReloadScreen()
{
	LOG->Debug( "ScreenTournamentOptions::ReloadScreen()" );
	SCREENMAN->SetNewScreen( m_sName );
}

/*
 * Copyright (c) 2008 BoXoRRoXoRs
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
