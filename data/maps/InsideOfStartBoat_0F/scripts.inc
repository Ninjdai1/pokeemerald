InsideOfStartBoat_0F_MapScripts::
	.byte 0

InsideOfStartBoat_0F_EventScript_BirchsBag::
	lock
//  textscreen textId:req, x:req, y:req, width:req, height:req, speed:req, bgColor:req, fontId:req, fontColor:req
  textscreen 0, 5, 5, 10, 5, 2, 0, 1, 1
	faceplayer
	setflag FLAG_SYS_POKEMON_GET
	setflag FLAG_RESCUED_BIRCH
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_ZIGZAGOON
	setobjectxy OBJ_EVENT_ID_PLAYER, 6, 13
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	special ChooseStarter
	waitstate
	//applymovement LOCALID_BIRCH, Route101_Movement_BirchApproachPlayer
	//waitmovement 0
	//msgbox Route101_Text_YouSavedMe, MSGBOX_DEFAULT
	special HealPlayerParty
	//setflag FLAG_HIDE_ROUTE_101_BIRCH_ZIGZAGOON_BATTLE
	//clearflag FLAG_HIDE_LITTLEROOT_TOWN_BIRCHS_LAB_BIRCH
	setflag FLAG_HIDE_INSIDE_OF_BOAT_BIRCH_STARTERS_BAG
	//setvar VAR_BIRCH_LAB_STATE, 2
	//setvar VAR_ROUTE101_STATE, 3
	clearflag FLAG_HIDE_MAP_NAME_POPUP
	//checkplayergender
	//call_if_eq VAR_RESULT, MALE, Route101_EventScript_HideMayInBedroom
	//call_if_eq VAR_RESULT, FEMALE, Route101_EventScript_HideBrendanInBedroom
	warp MAP_INSIDE_OF_START_BOAT_CABINS, 35, 16
	waitstate
	release
	end
