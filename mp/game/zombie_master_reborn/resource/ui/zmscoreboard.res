"resource/ui/zmscoreboard.res"
{
	"scores"
	{
		"ControlName"		"CZMClientScoreBoardDialog"
		"fieldName"		"scores"
		"xpos"		"140"
		"ypos"		"42"
		"wide"		"380"
		"tall"		"450"
		"autoResize"		"0"
		"visible"		"0"
		"enabled"		"1"
	}
	"RoundLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"RoundLabel"
		"xpos"		"32"
		"ypos"		"32"
		"wide"		"200"
		"tall"		"24"
		"autoResize"	"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"ROUND N/A"
		"font"		"ZMScoreboardItemBig"
		"textAlignment"		"north-west"
	}
	"ServerName"
	{
		"ControlName"		"Label"
		"fieldName"		"ServerName"
		"xpos"		"32"
		"ypos"		"45"
		"wide"		"200"
		"tall"		"24"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"north-west"
	}
	"MinorInfo"
	{
		"ControlName"		"Label"
		"fieldName"		"MinorInfo"
		"xpos"		"205"
		"ypos"		"34"
		"wide"		"120"
		"tall"		"32"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"north-east"
	}
	"PlayerList"
	{
		"ControlName"		"CZMListPanel"
		"fieldName"		"PlayerList"
		"xpos"		"26"
		"ypos"		"60"
		"wide"		"332"
		"tall"		"270"
		"autoResize"		"0"
		"visible"		"1"
		"enabled"		"1"
		
		"ListData"
		{
			"SectionZMName"
			{
				"default_font"		"ZMScoreboardHeaderBig"
				
				"Columns"
				{
					"name"
					{
						"width"		"150"
						"xoffset"		"22"
					}
				}
				"Items"
				{
					"1"
					{
						"name"		"ZOMBIE MASTER"
					}
				}
			}
			"SectionZMHeader"
			{
				"default_font"		"ZMScoreboardHeaderSmall"
				"default_itemheight"		"10"
				
				"Columns"
				{
					"name"
					{
						"width"		"120"
						"xoffset"		"35"
						"stretch_right"		"1"
					}
					"zmkills"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"ping"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"mutestatus"
					{
						"width"		"24"
						"colalign_right"		"1"
					}
					"importance"
					{
						"width"		"24"
						"colalign_right"		"1"
					}
				}
				"Items"
				{
					"1"
					{
						"name"		"Name"
						"zmkills"		"Score"
						"ping"		"Latency"
					}
				}
			}
			"SectionTeamZM"
			{
				"default_font"		"ZMScoreboardItemBig"
				"default_itemheight"		"19"
				
				"Columns"
				{
					"avatar"
					{
						"width"		"32"
						"is_image"		"1"
					}
					"name"
					{
						"width"		"100"
						"xoffset"		"4"
						"stretch_right"		"1"
					}
					"zmkills"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"ping"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"mutestatus"
					{
						"width"		"24"
						"colalign_right"		"1"
						"clickable"		"1"
						"is_image"		"1"
					}
					"importance"
					{
						"width"		"24"
						"colalign_right"		"1"
						"is_image"		"1"
						"is_tooltip"		"1"
					}
				}
			}
			"SectionHumanName"
			{
				"default_font"		"ZMScoreboardHeaderBig"
				"topmargin"		"10"
				
				"Columns"
				{
					"name"
					{
						"width"		"150"
						"xoffset"		"22"
					}
				}
				"Items"
				{
					"1"
					{
						"name"		"SURVIVORS"
					}
				}
			}
			"SectionHumanHeader"
			{
				"default_font"		"ZMScoreboardHeaderSmall"
				"default_itemheight"		"10"
				
				"Columns"
				{
					"name"
					{
						"width"		"120"
						"xoffset"		"35"
						"stretch_right"		"1"
					}
					"frags"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"deaths"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"ping"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"mutestatus"
					{
						"width"		"24"
						"colalign_right"		"1"
					}
					"importance"
					{
						"width"		"24"
						"colalign_right"		"1"
					}
				}
				"Items"
				{
					"1"
					{
						"name"		"Name"
						"frags"		"Score"
						"deaths"		"Deaths"
						"ping"		"Latency"
					}
				}
			}
			"SectionTeamHuman"
			{
				"default_font"		"ZMScoreboardItemNormal"
				"default_itemheight"		"16"
				
				"Columns"
				{
					"avatar"
					{
						"width"		"32"
						"is_image"		"1"
					}
					"name"
					{
						"width"		"100"
						"xoffset"		"4"
						"stretch_right"		"1"
					}
					"frags"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"deaths"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"ping"
					{
						"width"		"50"
						"align_center"		"1"
						"colalign_right"		"1"
					}
					"mutestatus"
					{
						"width"		"24"
						"colalign_right"		"1"
						"clickable"		"1"
						"is_image"		"1"
					}
					"importance"
					{
						"width"		"24"
						"colalign_right"		"1"
						"is_image"		"1"
						"is_tooltip"		"1"
					}
				}
			}
		}
	}
}
