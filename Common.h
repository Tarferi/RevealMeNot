#pragma once
#include "stdafx.h"
#include "stdlib.h"
#include <stdarg.h>
#include "string.h"
#include "miniz.h"
#include <cstdint>

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int8_t bool8;

#define MALLOC(target, type, size, failBlock) target = (type*) malloc(sizeof(type)*(size)); if(!target){failBlock};

#define MALLOC_N(target, type, size, failBlock) type* target; MALLOC(target, type, size, failBlock);

#define COMMON_CONSTR_SEC(name) name(unsigned char* name, unsigned int size, ReadBuffer* buffer) : Section(name, size, buffer) {};

#define COMMON_CONSTR_SEC_BS(name) name(unsigned char* name, unsigned int size, ReadBuffer* buffer) : BasicSection(name, size, buffer) {};

#define ENDS_WIDTH(name, suffix) (strlen(name) >= strlen(suffix) ? !strcmp(&(name[strlen(name) - strlen(suffix)]), (char*) suffix): false)

#define GET_CLONED_DATA_SZ(target, type, string, length, spec_sz, failBlock) MALLOC_N(target, type, length + spec_sz, failBlock);if(target){ memcpy(target, string, length*sizeof(type));}

#define GET_CLONED_DATA(target, type, string, length, failBlock) GET_CLONED_DATA_SZ(target, type, string, length, 0, failBlock)

#define GET_CLONED_STRING_LEN(target, string, length, failBlock) GET_CLONED_DATA_SZ(target, char, string, length, 1, failBlock); target[length] = 0

#define GET_CLONED_STRING(target, string, failBlock) GET_CLONED_STRING_LEN(target, string, strlen(string), failBlock)

#define ARRAY_DEFAULT_SIZE 64
#define ARRAY_INCREATE_FACTOR 2;

static int initValue = ARRAY_DEFAULT_SIZE;

#define LOG_ERROR(section, fmt, ...) fprintf(stderr, "[" section "] " fmt , __VA_ARGS__);


namespace Modifiers {

	enum Enum {
		AtLeast = 0,
		AtMost,
		IsSet,
		IsCleared,
		Set,
		Clear,
		Toggle,
		SetTo,
		Add,
		Subtract,
		Exactly,
		Randomize
	};

}

namespace ConditionType {
	enum Enum {
		None,
		CountdownTimer,
		Command,
		Bring,
		Accumulate,
		Kill,
		CommandTheMost,
		CommandsTheMostAt,
		MostKills,
		HighestScore,
		MostResources,
		Switch,
		ElapsedTime,
		MissionBriefing,
		Opponents,
		Deaths,
		CommandTheLeast,
		CommandTheLeastAt,
		LeastKills,
		LowestScore,
		LeastResources,
		Score,
		Always,
		Never
	};
}

namespace ActionType {
	enum Enum {
		None,
		Victory,
		Defeat,
		PreserveTrigger,
		Wait,
		PauseGame,
		UnpauseGame,
		Transmission,
		PlayWAV,
		DisplayTextMessage,
		CenterView,
		CreateUnitwithProperties,
		SetMissionObjectives,
		SetSwitch,
		SetCountdownTimer,
		RunAIScript,
		RunAIScriptAtLocation,
		LeaderBoardControl,
		LeaderBoardControlAt,
		LeaderBoardResources,
		LeaderBoardKills,
		LeaderBoardPoints,
		KillUnit,
		KillUnitAtLocation,
		RemoveUnit,
		RemoveUnitAtLocation,
		SetResources,
		SetScore,
		MinimapPing,
		TalkingPortrait,
		MuteUnitSpeech,
		UnmuteUnitSpeech,
		LeaderboardComputerPlayers,
		LeaderboardGoalControl,
		LeaderboardGoalControlAt,
		LeaderboardGoalResources,
		LeaderboardGoalKills,
		LeaderboardGoalPoints,
		MoveLocation,
		MoveUnit,
		LeaderboardGreed,
		SetNextScenario,
		SetDoodadState,
		SetInvincibility,
		CreateUnit,
		SetDeaths,
		Order,
		Comment,
		GiveUnitstoPlayer,
		ModifyUnitHitPoints,
		ModifyUnitEnergy,
		ModifyUnitShieldPoints,
		ModifyUnitResourceAmount,
		ModifyUnitHangerCount,
		PauseTimer,
		UnpauseTimer,
		Draw,
		SetAllianceStatus,
		DisableDebugMode,
		EnableDebugMode
	};
}

namespace Players {
	enum Enum {
		Player1 = 0,
		Player2,
		Player3,
		Player4,
		Player5,
		Player6,
		Player7,
		Player8,
		Player9,
		Player10,
		Player11,
		Player12,
		None,
		CurrentPlayer,
		Foes,
		Allies,
		NeutralPlayers,
		AllPlayers,
		Force1,
		Force2,
		Force3,
		Force4,
		Unused1,
		Unused2,
		Unused3,
		Unused4,
		NonAlliedVictoryPlayers,
		Max
	};
}

struct activeTile {
	unsigned char bVisibilityFlags;
	unsigned char bExploredFlags;
	unsigned char bWalkable : 1; // Set on tiles that can be walked on
	unsigned char bUnknown1 : 1; // Unused?
	unsigned char bUnwalkable : 1; // Set on tiles that can't be walked on
	unsigned char bUnknown2 : 3; // Unused?
	unsigned char bHasCreep : 1; // Set when creep occupies the area
	unsigned char bAlwaysUnbuildable : 1; // always unbuildable, like water
	unsigned char bGroundHeight : 3; // ground height
	unsigned char bCurrentlyOccupied : 1; // unbuildable but can be made buildable
	unsigned char bCreepReceeding : 1; // Set when the nearby structure supporting the creep is destroyed
	unsigned char bCliffEdge : 1; // Set if the tile is a cliff edge
	unsigned char bTemporaryCreep : 1; // Set when the creep occupying the area was created. Not set if creep tiles were preplaced. Used in drawing routine.
	unsigned char bUnknown3 : 1; // Unused?
};


#if _DEBUG
//#define DEBUG_LOG
#endif

#ifdef DEBUG_LOG

#define __LOG_SKIP(sectionName) _skip |= !strcmp(_section, sectionName);

#define LOG_INFO(section, fmt, ...) {\
	bool _skip=false;\
	char* _section = (char*) section;\
	if(!_skip){do { fprintf(stderr, "[" section "] " fmt "\n" , __VA_ARGS__); } while (0);}}

#else
#define LOG_INFO(section, fmt, ...)
#endif
template<typename type> class Array {

	class ArrayProxy {
		Array* array;
		int index;
	public:
		ArrayProxy(Array* array, int index) {
			this->array = array;
			this->index = index;
		}
		type operator= (type value) { array->set(index, value); return array->get(index); }
		operator type() { return array->get(index); }

	};

public:

	void remove(unsigned int index) {
		for (unsigned int i = index; i < this->dataSize - 1; i++) {
			this->rawData[i] = this->rawData[i + 1];
		}
		this->rawData[this->dataSize - 1] = (type) nullptr;
		this->dataSize--;
	}

	bool set(unsigned int index, type value) {
		bool error = false;
		if (index > this->size) {
			this->ensureAdditionalSize(this->size - index, &error);
		}
		if (error) {
			return false;
		}
		this->rawData[index] = value;
		if (index > this->dataSize) {
			this->dataSize = index + 1;
		}
		return true;
	}

	type get(unsigned int index) {
		return this->rawData[index];
	}

	unsigned int getSize() {
		return this->dataSize;
	}

	bool append(type value) {
		bool error = false;
		if (this->dataSize + sizeof(type) >= this->size) {
			this->ensureAdditionalSize(32 * sizeof(type), &error);
		}
		if (error) {
			return false;
		}
		this->rawData[this->dataSize] = value;
		this->dataSize++;
		return true;
	}

	void insert(unsigned int index, type value) {
		append(value);
		for (unsigned int i = this->dataSize - 1; i > index; i--) {
			this->rawData[i] = this->rawData[i - 1];
		}
		this->rawData[index] = value;
	}

	void freeItems() {
		for (unsigned int i = 0; i < this->getSize(); i++) {
			char* fn = this->get(i);
			free(fn);
		}
	}

	ArrayProxy operator[] (unsigned int index) {
		return ArrayProxy(this, index);
	}

	~Array() {
		if (this->rawData != nullptr) {
			free(this->rawData);
			this->rawData = nullptr;
		}
	}


private:

	type* rawData = nullptr;

	unsigned int size = 0;

	unsigned  int dataSize = 0;

	void ensureAdditionalSize(unsigned int size, bool* error) {
		if (this->dataSize + size > this->size) {
			if (this->rawData != nullptr) {
				void* toFree = this->rawData;
				unsigned int newSize = this->size * ARRAY_INCREATE_FACTOR;
				MALLOC(this->rawData, type, newSize, { free(toFree); *error = true; return; });
				memset(this->rawData, 0, newSize * sizeof(type));
				memcpy(this->rawData, toFree, this->size * sizeof(type));
				this->size = newSize;
				free(toFree);
			}
			else {
				unsigned int newSize = ARRAY_DEFAULT_SIZE;
				MALLOC(this->rawData, type, newSize, { *error = true; return; });
				memset(this->rawData, 0, newSize * sizeof(type));
				this->size = newSize;
				this->dataSize = 0;
			}
			this->ensureAdditionalSize(size, error);
		}
	}

};

#define LIBRARY_API __declspec(dllexport)

void compress(char* data, unsigned int length, char** outputData, unsigned int* outputLength, bool* error);

void decompress(char* data, unsigned int dataLength, char** outputData, unsigned int* outputLength, bool* error);
