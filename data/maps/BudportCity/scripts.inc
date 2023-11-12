BudportCity_MapScripts::
	.byte 0

BudportCity_EventScript_EVIVMaster::
	msgbox BudportCity_Text_EV_IV_Master, MSGBOX_NPC
  msgbox BudportCity_Text_EV_IV, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, BudportCity_EV_IV_Menu
	release
	end

BudportCity_EV_IV_Menu::
  special ShowEvIvScreen
  release
  end

BudportCity_Text_EV_IV_Master:
  .string "Pokémon have hidden stats.\n"
  .string "They're called EVs and IVs.\p"
  .string "EVs are Effort Values. They increase\n"
  .string "when you battle wild Pokémons or\l"
  .string "other trainers\p"
  .string "IVs are Individual Values. These are\n"
  .string "set when you capture a Pokémon and\l"
  .string "usually cannot be changed.$"

BudportCity_Text_EV_IV:
  .string "Do you want to see your\n"
  .string "Pokémons' EVs and IVs ?$"
