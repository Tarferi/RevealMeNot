// EUDUnrevealMe.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <string>
#include "Common.h"
#include "CHK.h"
#include "Storm.h"


#define EUD_DEATHTABLE 0x58A364
#define EPD_TO_ADDRESS(pid) ((pid * 4) + EUD_DEATHTABLE)
#define ADDRESS_TO_EPD(address) ((address - EUD_DEATHTABLE) / 4)
#define ADDRESS_DEATH_OF(player, unit) (EPD_TO_ADDRESS(((unit * 12) + player)))

#define EUD_TILESPTR 0x06D1260
#define EUD_CP 0x06509B0

#define GET_SECT(type, name, source, sourceName) type* name = (type*) source->getSection(sourceName);if(!name){return false;}

#define COND_DEATHS(data, player, modifier, number, unitID) data->ConditionType = ConditionType::Deaths; data->groupNumber = player; data->Comparision = modifier; data->Quantifier = number; data->UnitID = unitID
#define COND_SWITCH(data, switchIndex, modifier) data->ConditionType = ConditionType::Switch; data->Resource = switchIndex; data->Comparision = modifier
#define COND_MASK(data, value) data->locationNumber = value; data->Unused[0] = 'S'; data->Unused[1] = 'C';

#define ACT_SET_DEATHS(data, player, modifier, number, unitID) data->ActionType = ActionType::SetDeaths; data->Player = player; data->UnitsNumber = modifier; data->Group = number; data->UnitType = unitID
#define ACT_SET_SWITCH(data, swichIndex, modifier) data->ActionType = ActionType::SetSwitch; data->Group = switchIndex; data->UnitsNumber = modifier

#define BIT_ITER_BEGIN for (int flagIndex = 31; flagIndex >=0; flagIndex--) {unsigned int flag = 1 << flagIndex;
#define BIT_ITER_END }

#define SET_COND_SWITCH(index, switchIndex)  COND_SWITCH((&(trigger->conditions[index])), switchIndex, Modifiers::IsSet)
#define SET_COND_FLAG_PID(index, playerIndex) COND_DEATHS((&(trigger->conditions[index])), playerIndex, Modifiers::AtLeast, flag, 0)
#define SET_COND_FLAG_ADDR(index, address) SET_COND_FLAG_PID(index, ADDRESS_TO_EPD(address))

#define SET_ACT_PRESERVE(index) (&trigger->actions[index])->ActionType = ActionType::PreserveTrigger
#define SET_ACT_SWITCH_CLEAR(index, switchIndex)  ACT_SET_SWITCH((&(trigger->actions[index])), switchIndex, Modifiers::Clear)

#define SET_ACT_COUNT_DOWN_PID(index, playerIndex) ACT_SET_DEATHS((&(trigger->actions[index])), playerIndex, Modifiers::Subtract, flag, 0);
#define SET_ACT_COUNT_DOWN_ADDR(index, address) SET_ACT_COUNT_DOWN_PID(index, ADDRESS_TO_EPD(address))

#define SET_ACT_COUNT_UP_PID(index, playerIndex) ACT_SET_DEATHS((&(trigger->actions[index])), playerIndex, Modifiers::Add, flag, 0);
#define SET_ACT_COUNT_UP_ADDR(index, address) SET_ACT_COUNT_UP_PID(index, ADDRESS_TO_EPD(address))

#define SET_COND_HAS_BIT_PID(index, playerIndex) COND_DEATHS((&(trigger->conditions[index])), playerIndex, Modifiers::AtLeast, 1, 0);COND_MASK((&(trigger->conditions[index])), flag)
#define SET_COND_HAS_BIT_ADDR(index, address) SET_COND_HAS_BIT_PID(index, ADDRESS_TO_EPD(address))

#define CREATE_TRIGGER MALLOC_N(trigger, Trigger, 1, { return false; });if (!triggers->append(trigger)) {free(trigger);return false;}memset(trigger, 0, sizeof(Trigger));trigger->players[Players::AllPlayers] = 1;

#define PTR_CPBACKUP ADDRESS_DEATH_OF(Players::Player12, 5)
#define PTR_ITEAR_DEREF_PID ADDRESS_DEATH_OF(Players::Player12, 3)

bool saveCP(Array<Trigger*>* triggers, unsigned int switchIndex) {
	CREATE_TRIGGER;
	SET_COND_SWITCH(0, switchIndex);
	ACT_SET_DEATHS((&(trigger->actions[1])), ADDRESS_TO_EPD(PTR_CPBACKUP), Modifiers::SetTo, 0, 0);
	SET_ACT_PRESERVE(2);

	BIT_ITER_BEGIN;
		CREATE_TRIGGER;
		SET_COND_SWITCH(0, switchIndex);
		SET_COND_HAS_BIT_ADDR(1, EUD_CP);
		SET_ACT_COUNT_UP_ADDR(0, PTR_CPBACKUP);
		SET_ACT_PRESERVE(1);
	BIT_ITER_END;

	return true;
}

bool restoreCP(Array<Trigger*>* triggers, unsigned int switchIndex) {
	CREATE_TRIGGER;
	SET_COND_SWITCH(0, switchIndex);
	ACT_SET_DEATHS((&(trigger->actions[0])), ADDRESS_TO_EPD(EUD_CP), Modifiers::SetTo, 0, 0);
	SET_ACT_PRESERVE(1);

	BIT_ITER_BEGIN;
		CREATE_TRIGGER;
		SET_COND_SWITCH(0, switchIndex);
		SET_COND_HAS_BIT_ADDR(1, PTR_CPBACKUP);
		
		SET_ACT_COUNT_UP_ADDR(0, EUD_CP);
		SET_ACT_PRESERVE(1);
	BIT_ITER_END;

	return true;
}

bool addTerminatorTrigger(Array<Trigger*>* triggers, unsigned int switchIndex) {
	CREATE_TRIGGER;
	SET_COND_SWITCH(0, switchIndex);
	SET_ACT_SWITCH_CLEAR(0, switchIndex);
	SET_ACT_PRESERVE(1);
	return true;
}

struct ActionSequence {
	Array<Trigger*>* triggers;
	Trigger* trigger;
	unsigned int nextActionToUse;
};

bool getNextSequence(ActionSequence* sequence, unsigned int switchIndex, Action** action) {
	if (sequence->nextActionToUse == 62) {
		(&(sequence->trigger->actions)[63])->ActionType = ActionType::PreserveTrigger;
		*action = &(sequence->trigger)->actions[62];
		sequence->nextActionToUse = 0; 
		Array<Trigger*>* triggers = sequence->triggers; 
		CREATE_TRIGGER; 

		SET_COND_SWITCH(0, switchIndex); 
		sequence->trigger = trigger; 
	} else {
		sequence->nextActionToUse++; 
		*action = &(sequence->trigger)->actions[sequence->nextActionToUse - 1];
	}
	return true;
}

bool finishSequence(ActionSequence* sequence) {
	if (sequence->nextActionToUse == 0) { // Last trigger empty, remove it
		if (sequence->triggers->getSize() > 0) {
			sequence->triggers->remove(sequence->triggers->getSize() - 1);
		}
	} else { 
		Action* action = &(sequence->trigger)->actions[sequence->nextActionToUse];
		action->ActionType = ActionType::PreserveTrigger;
	}
	return true;
}

#define GET_NEXT_ACTION(action, switchIndex) Action* action; if(!getNextSequence(sequence, switchIndex, &action)){return false;}

bool setCPToTilePtr(Array<Trigger*>* triggers, unsigned int switchIndex) {
	
	// Set CP to 0
	CREATE_TRIGGER;
	SET_COND_SWITCH(0, switchIndex);
	ACT_SET_DEATHS((&(trigger->actions[0])), ADDRESS_TO_EPD(EUD_CP), Modifiers::SetTo, 0, 0);
	SET_ACT_PRESERVE(1);

	// Copy PTR_ITEAR_DEREF_PID to CP
	BIT_ITER_BEGIN;
		CREATE_TRIGGER;
		SET_COND_SWITCH(0, switchIndex);
		SET_COND_HAS_BIT_ADDR(1, PTR_ITEAR_DEREF_PID);

		SET_ACT_COUNT_UP_ADDR(0, EUD_CP);
		SET_ACT_PRESERVE(1);
	BIT_ITER_END;

	return true;
}

bool incCP(ActionSequence* sequence, unsigned int addValue, unsigned int switchIndex) {
	GET_NEXT_ACTION(setAct, switchIndex);
	ACT_SET_DEATHS(setAct, ADDRESS_TO_EPD(EUD_CP), Modifiers::Add, addValue, 0);

	return true;
}

bool derefTilePtr(Array<Trigger*>* triggers, unsigned int switchIndex) {
	
	{
		// Set targets to 0
		CREATE_TRIGGER;
		SET_COND_SWITCH(0, switchIndex);
		ACT_SET_DEATHS((&(trigger->actions[0])), ADDRESS_TO_EPD(PTR_ITEAR_DEREF_PID), Modifiers::SetTo, 0, 0);
		SET_ACT_PRESERVE(1);
	}

	BIT_ITER_BEGIN;
		CREATE_TRIGGER;
		SET_COND_SWITCH(0, switchIndex);
		SET_COND_HAS_BIT_ADDR(1, EUD_TILESPTR);
		
		// Divide by 4
		unsigned int _flag = flag;
		if (flag >= 0b100) {
			unsigned int flag = _flag >> 2;
			SET_ACT_COUNT_UP_ADDR(0, PTR_ITEAR_DEREF_PID);
			if (flag == 1) {
				break;
			}
		}
		SET_ACT_PRESERVE(1);
	BIT_ITER_END;
	
	// Add PID offset
	CREATE_TRIGGER;
	SET_COND_SWITCH(0, switchIndex);
	ACT_SET_DEATHS((&(trigger->actions[0])), ADDRESS_TO_EPD(PTR_ITEAR_DEREF_PID), Modifiers::Subtract, EUD_DEATHTABLE / 4, 0);
	SET_ACT_PRESERVE(1);

	return true;
}

void deleteTrigs(Array<Trigger*>* triggers) {
	for (unsigned int i = 0; i < triggers->getSize(); i++) {
		Trigger* trig = triggers->get(i);
		free(trig);
	}
}

bool restoreTile(ActionSequence* sequence, unsigned int switchIndex, unsigned char defaultValue) {
	GET_NEXT_ACTION(action, switchIndex);

	// Overwrite to default values
	ACT_SET_DEATHS(action, Players::CurrentPlayer, Modifiers::SetTo, defaultValue << 8, 0);

	action->SourceLocation = 0xff00; // Affect lowest 2 bytes only

	// Enable masking
	action->Unused[1] = 'S';
	action->Unused[2] = 'C';

	return true;
}

bool restoreFOW(Array<Trigger*>* triggers, unsigned int switchIndex, unsigned int tilesTotal, unsigned char* tilesDefaultValues, unsigned int tilesDefaultValuesLength, unsigned char defaultValueX, bool useDefaultValue) {
	
	// CP points to first tile
	ActionSequence seq;
	seq.nextActionToUse = 0;
	seq.triggers = triggers;
	CREATE_TRIGGER;
	SET_COND_SWITCH(0, switchIndex);
	seq.trigger = trigger;


	// Reset every tile
	for (unsigned int i = 0; i < tilesTotal; i++) {
		unsigned char defaultValue = useDefaultValue ? defaultValueX : i < tilesDefaultValuesLength ? tilesDefaultValues[i] : 0xff;

		// Set the value
		if (!restoreTile(&seq, switchIndex, defaultValue)) { return false; }

		// Increment CP
		if (!incCP(&seq, /* sizeof(activeTile) */ 1, switchIndex)) { return false; }

	}

	if (!finishSequence(&seq)) { return false; }

	return true;
}

#define TRIG_SPRINTFC(buffer) {bool error = false; wb->writeFixedLengthString((unsigned char*)buffer, &error); if(error){return false;}}
#define TRIG_SPRINTF(fmt, ...) {char buffer[1024]; sprintf_s(buffer, fmt, __VA_ARGS__); TRIG_SPRINTFC(buffer);}


bool printTrigger(Trigger* trig, WriteBuffer* wb) {
	TRIG_SPRINTFC("Trigger(\"All Players\"){\r\n");
	TRIG_SPRINTFC("Conditions:\r\n");
	for (unsigned int i = 0; i < 16; i++) {
		Condition* c = &(trig->conditions[i]);
		if (c->ConditionType == ConditionType::Switch) {
			TRIG_SPRINTF("\tSwitch(\"Switch%d\", Set);\r\n", c->Resource);
		} else if (c->ConditionType == ConditionType::Deaths) {
			if (c->Unused[0] == 'S' && c->Unused[1] == 'C') {
				TRIG_SPRINTF("\tMasked MemoryAddr(%d, At Least, 1, %d);\r\n", EPD_TO_ADDRESS(c->groupNumber), c->locationNumber);
			} else {
				TRIG_SPRINTF("\tMemoryAddr(%d, At Least, 1);\r\n", EPD_TO_ADDRESS(c->groupNumber));
			}
		} else if (c->ConditionType == 0) {
			break;
		} else {
			return false;
		}
	}
	TRIG_SPRINTFC("Actions:\r\n");
	for (unsigned int i = 0; i < 64; i++) {
		Action* a = &(trig->actions[i]);
		if (a->ActionType == ActionType::SetSwitch) {
			TRIG_SPRINTF("\tSet Switch(\"Switch%d\", clear);\r\n", a->Group);
		} else if (a->ActionType == ActionType::PreserveTrigger) {
			TRIG_SPRINTFC("\tPreserve Trigger();\r\n");
		} else if (a->ActionType == ActionType::SetDeaths) {
			if (a->Unused[1] == 'S' && a->Unused[2] == 'C') {
				TRIG_SPRINTF("\tMasked MemoryAddr(%d, %s, %d, %d);\r\n", EPD_TO_ADDRESS(a->Player), (a->UnitsNumber == Modifiers::SetTo ? "Set to" : (a->UnitsNumber == Modifiers::Add ? "Add" : "Subtract")), a->Group, a->SourceLocation);
			} else {
				TRIG_SPRINTF("\tMemoryAddr(%d, %s, %d);\r\n", EPD_TO_ADDRESS(a->Player), (a->UnitsNumber == Modifiers::SetTo ? "Set to" : (a->UnitsNumber == Modifiers::Add ? "Add" : "Subtract")), a->Group);
			}
		} else if (a->ActionType == 0) {
			break;
		} else {
			return false;
		}
	}
	TRIG_SPRINTFC("}\r\n\r\n//-----------------------------------------------------------------//\r\n\r\n");
	return true;
}

bool addFogRemove(CHK* chk, unsigned char switchIndex, WriteBuffer* output, unsigned char fogValue) {
	GET_SECT(Section_MASK, MASK, chk, "MASK");
	GET_SECT(Section_DIM_, DIM, chk, "DIM ");
	GET_SECT(Section_TRIG, TRIG, chk, "TRIG");
	GET_SECT(Section_VER_, VER, chk, "VER ");

	if (VER->version == 59 || VER->version == 63 || VER->version == 205) {
		VER->version = VER->version + 1;
	}

	unsigned int width = DIM->width;
	unsigned int height = DIM->height;
	
	Array<Trigger*> triggers;

	// Backup current player
	if (!saveCP(&triggers, switchIndex)) { deleteTrigs(&triggers); return false; }

	// Deref tiles ptr
	if (!derefTilePtr(&triggers, switchIndex)) { deleteTrigs(&triggers); return false; }

	// set current player to deref tiles ptr
	if (!setCPToTilePtr(&triggers, switchIndex)) { deleteTrigs(&triggers); return false; }

	// restore FOW flags
	if (!restoreFOW(&triggers, switchIndex, width * height, MASK->data, MASK->size, fogValue, true)) { deleteTrigs(&triggers); return false; }

	// Restore current player
	if (!restoreCP(&triggers, switchIndex)) { deleteTrigs(&triggers); return false; }

	// Terminator
	if (!addTerminatorTrigger(&triggers, switchIndex)) { deleteTrigs(&triggers); return false; }

	// Insert all trigger into the map
	for (unsigned int i = 0; i < triggers.getSize(); i++) {
		Trigger* trig = triggers.get(i);
		if (output != nullptr) {
			if (!printTrigger(trig, output)) {
				deleteTrigs(&triggers);
				return false;
			}
		} else {
			if (!TRIG->triggers.append(trig)) {
				for (unsigned int o = i; o < triggers.getSize(); o++) {
					Trigger* freeTrig = triggers.get(i);
					free(freeTrig);
				}
				return false;
			}
		}
	}
	if (output != nullptr) {
		deleteTrigs(&triggers);
	}
	return true;
}

int main(int argc, char** argv) {

	bool skip = false;
	char* inputFile = nullptr;
	char* switchIndex = nullptr;
	char* outputFile = nullptr;
	bool showHelp = true;
	bool replaceInTriggers = false;

#define ARG_IS(longVersion, shortVersion) !strcmp(arg, shortVersion) || !strcmp(arg, longVersion)
#define SET_ARG(item, err) if(item != nullptr || argI + 1 == argc){fprintf(stderr, err "\n");return 2;};item = argv[argI + 1];skip=true;showHelp = false;continue;
#define ARG(longVersion, shortVersion, item, err) if(ARG_IS(longVersion, shortVersion)) {SET_ARG(item,err);}

	for (int argI = 1; argI < argc; argI++) {
		if (skip) {
			skip = false;
			continue;
		}
		char* arg = argv[argI];
		ARG("-i", "--input", inputFile, "Invalid option for input file");
		ARG("-o", "--output", outputFile, "Invalid option for output file");
		ARG("-s", "--switch", switchIndex, "Invalid option for switch index");
		if (ARG_IS("-h", "--help")) {
			showHelp = true;
			break;
		}
	}
	unsigned char switchIndexC = switchIndex == nullptr ? 0 : atoi(switchIndex);

	if (showHelp) {
		fprintf(stderr, "RevealMeNot by iThief\r\n");
		fprintf(stderr, "\r\n\r\nUsage:\r\n");
		fprintf(stderr, "\t-i <input_file>      Input map file\r\n");
		fprintf(stderr, "\t-o <output_file>     Optional, output map file. If not present, outputs generated triggers\r\n");
		fprintf(stderr, "\t-s <switch_index>    Switch index to use. Set the switch to execute unreveal triggers. Switch is cleared automatically\r\n");
		return 3;
	}

	bool err = false;
	Storm storm(&err);
	if (err) {
		LOG_ERROR("UnrevealMe", "Cannot decode storm library");
		return 1;
	}
	MapFile* mf = storm.readSCX(inputFile, &err);
	if (err) {
		LOG_ERROR("UnrevealMe", "Cannot open \"%s\"", inputFile);
		return 1;
	}
	WriteBuffer* wb;
	wb = outputFile == nullptr ? new WriteBuffer() : nullptr;

	if (!addFogRemove(mf->getCHK(), switchIndexC, wb, 0) || !addFogRemove(mf->getCHK(), switchIndexC + 1, wb, 0xff)) {
		LOG_ERROR("UnrevealMe", "Could not process file \"%s\"", inputFile);
		delete mf;
		if (wb != nullptr) {
			delete wb;
			wb = nullptr;
		}
		return 1;
	}

	if (wb != nullptr) {
		bool error = false;
		wb->writeByte(0, &error);
		if (error) {
			LOG_ERROR("UnrevealMe", "Could not process file \"%s\"", inputFile);
			delete mf;
			delete wb;
			wb = nullptr;
			return 1;
		}

		unsigned char* data;
		unsigned int length;
		wb->getWrittenData(&data, &length);
		printf("%s", data);

		delete wb;
		wb = nullptr;
	} else {
		if (!storm.writeSCX(outputFile, mf)) {
			LOG_ERROR("UnrevealMe", "Cannot write \"%s\"", outputFile);
			return 1;
		}
		delete mf;
		LOG_INFO("UnrevealMe", "Done");
	}
	return 0;
}

