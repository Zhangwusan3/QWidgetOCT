#pragma once
#ifndef ATSDIGITIZER_H
#define ATSDIGITIZER_H
#include "AlazarApi.h"

class ATSdigitizer {

public:
	U32 systemId;// = 1;
	U32 boardId; //= 1;
	RETURN_CODE retCode;
	HANDLE boardHandle;

public:
	ATSdigitizer(U32 msysID = 1, U32 mboaID = 1);
	~ATSdigitizer();


public:
	int AlazarDLLCfg();
	int AlazarDLLAcq();

};



#endif // !ATSDIGITIZER_H

