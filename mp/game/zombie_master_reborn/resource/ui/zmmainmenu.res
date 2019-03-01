"resource/ui/zmmainmenu.res"
{
	"ZMMainMenu"
	{
		"ControlName"		"CZMMainMenu"
		"fieldName"		"ZMMainMenu"
		"xpos"		"0"
		"ypos"		"0"
		"wide"		"640"
		"tall"		"480"
		"visible"		"1"
		"enabled"		"1"
		
		// The child from which the bottom strip is painted
		"bottom_strip_start"		"ServerButton"
	}
	"ResumeButton"
	{
		"ControlName"		"CZMMainMenuButton"
		"fieldName"		"ResumeButton"
		"xpos"		"30"
		"ypos"		"240"
		"wide"		"180"
		"tall"		"32"
		"autoResize"		"0"
		"visible"		"0"
		"enabled"		"1"
		"labelText"		"#GameUI_GameMenu_ResumeGame"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"command"		"ResumeGame"
		"font"		"ZMMainMenuButton"
		"allcaps"		"1"
		"sound_armed"		"zmr_mainmenu/buttonrollover.wav"
		"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
		
		"onlyingame"		"1"
	}
	"DisconnectButton"
	{
		"ControlName"		"CZMMainMenuButton"
		"fieldName"		"DisconnectButton"
		"xpos"		"30"
		"ypos"		"272"
		"wide"		"180"
		"tall"		"32"
		"autoResize"		"0"
		"visible"		"0"
		"enabled"		"1"
		"labelText"		"#GameUI_GameMenu_Disconnect"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"command"		"Disconnect"
		"font"		"ZMMainMenuButton"
		"allcaps"		"1"
		"sound_armed"		"zmr_mainmenu/buttonrollover.wav"
		"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
		
		"onlyingame"		"1"
	}
	"ServerButton"
	{
		"ControlName"		"CZMMainMenuButton"
		"fieldName"		"ServerButton"
		"xpos"		"30"
		"ypos"		"400"
		"wide"		"100"
		"tall"		"40"
		"autoResize"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#GameUI_Play"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"		"ZMMainMenuButton"
		"default"		"1"
		"allcaps"		"1"
		"sound_armed"		"zmr_mainmenu/buttonrollover.wav"
		
		"imagematerial"		"zmr_mainmenu/menuicon_play"
		"imagealign"		"0"
		"subbuttons"
		{
			"createserver"
			{
				"fieldName"		"CreateServer"
				"allcaps"		"1"
				"labelText"		"#GameUI_CreateServer"
				"command"		"OpenCreateMultiplayerGameDialog"
				
				"sound_armed"		"zmr_mainmenu/subbuttonrollover.wav"
				"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
			}
			"playonline"
			{
				"fieldName"		"PlayOnline"
				"allcaps"		"1"
				"labelText"		"#GameUI_GameMenu_FindServers"
				"command"		"OpenServerBrowser"
				
				"sound_armed"		"zmr_mainmenu/subbuttonrollover.wav"
				"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
			}
			"playtutorial"
			{
				"fieldName"		"PlayTutorial"
				"allcaps"		"1"
				"labelText"		"PLAY TUTORIAL"
				"command"		""
				"enabled"		"0"
				
				"sound_armed"		"zmr_mainmenu/subbuttonrollover.wav"
				"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
			}
		}
	}
	"OptionsButton"
	{
		"ControlName"		"CZMMainMenuButton"
		"fieldName"		"OptionsButton"
		"xpos"		"130"
		"ypos"		"400"
		"wide"		"100"
		"tall"		"40"
		"autoResize"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#GameUI_GameMenu_Options"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"		"ZMMainMenuButton"
		"allcaps"		"1"
		"sound_armed"		"zmr_mainmenu/buttonrollover.wav"
		
		"imagematerial"		"zmr_mainmenu/menuicon_gear"
		"subbuttons"
		{
			"modoptions"
			{
				"fieldName"		"ModOptions"
				"allcaps"		"1"
				"labelText"		"Mod Options"
				"command"		"engine OpenZMOptions"
				
				"sound_armed"		"zmr_mainmenu/subbuttonrollover.wav"
				"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
			}
			"defoptions"
			{
				"fieldName"		"DefaultOptions"
				"allcaps"		"1"
				"labelText"		"#GameUI_Options"
				"command"		"engine OpenZMGameUIOptions"
				
				"sound_armed"		"zmr_mainmenu/subbuttonrollover.wav"
				"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
			}
		}
	}
	"CallVoteButton"
	{
		"ControlName"		"CZMMainMenuButton"
		"fieldName"		"CallVoteButton"
		"xpos"		"250"
		"ypos"		"400"
		"wide"		"150"
		"tall"		"40"
		"autoResize"		"0"
		"visible"		"0"
		"enabled"		"1"
		"labelText"		"CALL VOTE"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"imagematerial"		"zmr_mainmenu/menuicon_vote"
		"font"		"ZMMainMenuButton"
		"allcaps"		"1"
		"sound_armed"		"zmr_mainmenu/buttonrollover.wav"
		
		"onlyingame"		"1"
		"subbuttons"
		{
			"restartround"
			{
				"fieldName"		"RestartRound"
				"allcaps"		"1"
				"labelText"		"Restart Round"
				"command"		"engine callvote ZMVoteRoundRestart"
				
				"sound_armed"		"zmr_mainmenu/subbuttonrollover.wav"
				"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
			}
		}
	}
	"ExitButton"
	{
		"ControlName"		"CZMMainMenuButton"
		"fieldName"		"ExitButton"
		"xpos"		"r110"
		"ypos"		"400"
		"wide"		"100"
		"tall"		"40"
		"autoResize"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#GameUI_GameMenu_Quit"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"command"		"Quit"
		"font"		"ZMMainMenuButton"
		"allcaps"		"1"
		"sound_armed"		"zmr_mainmenu/buttonrollover.wav"
		"sound_released"		"zmr_mainmenu/buttonclickrelease.wav"
		
		//"imagematerial"		"zmr_mainmenu/menuicon_test"
		//"imagealign"		"1"
	}
	"ImagePanel"
	{
		"ControlName"		"CZMMainMenuImagePanel"
		"fieldName"		"ImagePanel"
		"fillcolor"		"0 0 0 0"
		//"image"		""
		"drawcolor"		"ZMMainMenuBg"
		"scaleImage"		"1"
		"xpos"		"r220"
		"ypos"		"25"
		"wide"		"190"
		"tall"		"380"
		"visible"		"1"
		"enabled"		"1"
		"mouseinputenabled"		"0"
		"alignbottom"		"1"
		"images"
		{
			"hulk"
			{
				"img"		"zmr_mainmenu/portrait/hulk"
			}
			//"special"
			//{
			//	"img"		"zmr_mainmenu/portrait/test"
			//	"event"		"0"
			//}
		}
	}
	"IconLinks"
	{
		"ControlName"		"CZMMainMenuContactButtonList"
		"fieldName"		"IconLinks"
		"xpos"		"r64"
		"ypos"		"440"
		"wide"		"100"
		"tall"		"30"
		"visible"		"1"
		"enabled"		"1"
		"links"
		{
			"discord"
			{
				"url"		"https://discord.gg/zS8qHN9"
				"icon"		"zmr_mainmenu/icon_discord"
			}
			"steam"
			{
				"url"		"https://steamcommunity.com/groups/zmreborn"
				"icon"		"zmr_mainmenu/icon_steam"
			}
		}
	}
}
