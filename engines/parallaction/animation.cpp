/* ScummVM - Scumm Interpreter
 * Copyright (C) 2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "parallaction/disk.h"
#include "parallaction/parallaction.h"
#include "parallaction/graphics.h"
#include "parallaction/music.h"
#include "parallaction/parser.h"
#include "parallaction/zone.h"


namespace Parallaction {


#define INST_ON 						1
#define INST_OFF						2
#define INST_X							3
#define INST_Y							4
#define INST_Z							5
#define INST_F							6
#define INST_LOOP						7
#define INST_ENDLOOP					8
#define INST_SHOW						9
#define INST_INC						10
#define INST_DEC						11
#define INST_SET						12
#define INST_PUT						13
#define INST_CALL						14
#define INST_WAIT						15
#define INST_START						16
#define INST_SOUND						17
#define INST_MOVE						18
#define INST_END						1000


void	wrapLocalVar(LocalVariable *local);


LValue	getLValue(Instruction *inst, char *str, LocalVariable *locals, Animation *a);


uint16	_numLocals = 0;
char	_localNames[10][10];

Animation *Parallaction::findAnimation(const char *name) {

	Animation *v4 = (Animation*)_animations._next;

	while (v4) {
		if (!scumm_stricmp(name, v4->_zone._label._text)) return v4;
		v4 = (Animation*)v4->_zone._next;
	}

	return NULL;
}


Animation *Parallaction::parseAnimation(Script& script, Node *list, char *name) {
//	printf("parseAnimation(%s)\n", name);

	Animation *vD0 = new Animation;

	vD0->_zone._label._text = (char*)malloc(strlen(name)+1);
	strcpy(vD0->_zone._label._text, name);

	addNode(list, &vD0->_zone);

	fillBuffers(script, true);
	while (scumm_stricmp(_tokens[0], "endanimation")) {
//		printf("token[0] = %s\n", _tokens[0]);

		if (!scumm_stricmp(_tokens[0], "script")) {
			loadProgram(vD0, _tokens[1]);
		}
		if (!scumm_stricmp(_tokens[0], "commands")) {
			vD0->_zone._commands = parseCommands(script);
		}
		if (!scumm_stricmp(_tokens[0], "type")) {
			if (_tokens[2][0] != '\0') {
				vD0->_zone._type = ((4 + searchTable(_tokens[2], const_cast<const char **>(_objectsNames))) << 16) & 0xFFFF0000;
			}
			int16 _si = searchTable(_tokens[1], _zoneTypeNames);
			if (_si != -1) {
				vD0->_zone._type |= 1 << (_si-1);
				if (((vD0->_zone._type & 0xFFFF) != kZoneNone) && ((vD0->_zone._type & 0xFFFF) != kZoneCommand)) {
					parseZoneTypeBlock(script, &vD0->_zone);
				}
			}
		}
		if (!scumm_stricmp(_tokens[0], "label")) {
			_vm->_gfx->makeCnvFromString(&vD0->_zone._label._cnv, _tokens[1]);
		}
		if (!scumm_stricmp(_tokens[0], "flags")) {
			uint16 _si = 1;

			do {
				byte _al = searchTable(_tokens[_si], _zoneFlagNames);
				_si++;
				vD0->_zone._flags |= 1 << (_al - 1);
			} while (!scumm_stricmp(_tokens[_si++], "|"));
		}
		if (!scumm_stricmp(_tokens[0], "file")) {
			char vC8[200];
			strcpy(vC8, _tokens[1]);
			if (_engineFlags & kEngineTransformedDonna) {
				if (!scumm_stricmp(_tokens[1], "donnap") || !scumm_stricmp(_tokens[1], "donnapa")) {
					strcat(vC8, "tras");
				}
			}
			vD0->_cnv = _disk->loadFrames(vC8);
		}
		if (!scumm_stricmp(_tokens[0], "position")) {
			vD0->_zone._left = atoi(_tokens[1]);
			vD0->_zone._top = atoi(_tokens[2]);
			vD0->_z = atoi(_tokens[3]);
		}
		if (!scumm_stricmp(_tokens[0], "moveto")) {
			vD0->_zone._moveTo.x = atoi(_tokens[1]);
			vD0->_zone._moveTo.y = atoi(_tokens[2]);
		}

		fillBuffers(script, true);
	}

	vD0->_zone._oldLeft = -1000;
	vD0->_zone._oldTop = -1000;

	vD0->_zone._flags |= 0x1000000;

	return vD0;
}



void  freeScript(Program *program) {

	if (!program) return;

	delete[] program->_locals;
	freeNodeList(program);

	return;
}



void Parallaction::freeAnimations() {
	Animation *v4 = (Animation*)_animations._next;
	while (v4) {
		freeScript(v4->_program);
		_vm->_gfx->freeCnv(v4->_cnv);
		if (v4->_cnv) delete v4->_cnv;
		v4 = (Animation*)v4->_zone._next;

		// TODO: delete Animation
	}

	return;
}



void jobDisplayAnimations(void *parm, Job *j) {
//	printf("jobDisplayAnimations()...\n");

	Animation *v18 = (Animation*)_vm->_animations._next;
	StaticCnv v14;

	uint16 _si = 0;

	for ( ; v18; v18 = (Animation*)v18->_zone._next) {

		if ((v18->_zone._flags & kFlagsActive) && ((v18->_zone._flags & kFlagsRemove) == 0))   {
			v14._width = v18->width();
			v14._height = v18->height();
			v14._data0 = v18->getFrameData(v18->_frame);
//			v14._data1 = v18->_cnv->field_8[v18->_frame];

			if (v18->_zone._flags & kFlagsNoMasked)
				_si = 3;
			else
				_si = _vm->_gfx->queryMask(v18->_zone._top + v18->height());

//			printf("jobDisplayAnimations %s, x: %i, y: %i, w: %i, h: %i\n", v18->_zone._name, v18->_zone._left, v18->_zone._top, v14._width, v14._height);
			_vm->_gfx->blitCnv(&v14, v18->_zone._left, v18->_zone._top, _si, Gfx::kBitBack);

		}

		if (((v18->_zone._flags & kFlagsActive) == 0) && (v18->_zone._flags & kFlagsRemove))   {
			v18->_zone._flags &= ~kFlagsRemove;
			v18->_zone._oldLeft = -1000;
		}

		if ((v18->_zone._flags & kFlagsActive) && (v18->_zone._flags & kFlagsRemove))	{
			v18->_zone._flags &= ~kFlagsActive;
			v18->_zone._flags |= kFlagsRemove;
		}

	}

//	  printf("done\n");

	return;
}


void jobEraseAnimations(void *arg_0, Job *j) {
	debugC(3, kDebugJobs, "jobEraseAnimations");

	Animation *a = (Animation*)_vm->_animations._next;

	for (; a; a=(Animation*)a->_zone._next) {

		if (((a->_zone._flags & kFlagsActive) == 0) && ((a->_zone._flags & kFlagsRemove) == 0)) continue;

		Common::Rect r(a->width(), a->height());
		r.moveTo(a->_zone._oldLeft, a->_zone._oldTop);
		_vm->_gfx->restoreBackground(r);

		if (arg_0) {
			a->_zone._oldLeft = a->_zone._left;
			a->_zone._oldTop = a->_zone._top;
		}

	}

//	printf("done\n");

	return;
}


void Parallaction::loadProgram(Animation *a, char *filename) {
//	printf("loadProgram(%s)\n", filename);

	Script *script = _disk->loadScript(filename);

	_numLocals = 0;

	fillBuffers(*script);

	a->_program = new Program;

	a->_program->_locals = new LocalVariable[10];
	Node *vD0 = a->_program;

	Instruction *vCC = new Instruction;

	while (scumm_stricmp(_tokens[0], "endscript")) {

		parseScriptLine(vCC, a, a->_program->_locals);
		addNode(vD0, vCC);
		vD0 = vCC;

		vCC = new Instruction;
		fillBuffers(*script);
	}

	vCC->_index = INST_END;
	addNode(vD0, vCC);

	delete script;

	a->_program->_ip = (Instruction*)a->_program->_next;

	return;
}






void Parallaction::parseScriptLine(Instruction *inst, Animation *a, LocalVariable *locals) {
//	printf("parseScriptLine()\n");

	if (_tokens[0][1] == '.') {
		_tokens[0][1] = '\0';
		a = findAnimation(&_tokens[0][2]);
	}

	if (_tokens[1][1] == '.') {
		_tokens[1][1] = '\0';
		a = findAnimation(&_tokens[1][2]);
	}

	int16 _si = searchTable(_tokens[0], _instructionNames);
	inst->_index = _si;

//	printf("token[0] = %s (%i)\n", _tokens[0], inst->_index);

	switch (inst->_index) {
	case INST_ON:	// on
	case INST_OFF:	// off
	case INST_START:	// start
		if (!scumm_stricmp(_tokens[1], a->_zone._label._text)) {
			inst->_opBase._a = a;
		} else {
			inst->_opBase._a = findAnimation(_tokens[1]);
		}
		break;

	case INST_LOOP: // loop
		inst->_opBase._loopCounter = getLValue(inst, _tokens[1], locals, a);
		break;

	case INST_X:	// x
		inst->_opA._pvalue = &a->_zone._left;
		inst->_opB = getLValue(inst, _tokens[1], locals, a);
		break;

	case INST_Y:	// y
		inst->_opA._pvalue = &a->_zone._top;
		inst->_opB = getLValue(inst, _tokens[1], locals, a);
		break;

	case INST_Z:	// z
		inst->_opA._pvalue = &a->_z;
		inst->_opB = getLValue(inst, _tokens[1], locals, a);
		break;

	case INST_F:	// f
		inst->_opA._pvalue = &a->_frame;
		inst->_opB = getLValue(inst, _tokens[1], locals, a);
		break;

	case INST_INC:	// inc
	case INST_DEC:	// dec
		if (!scumm_stricmp(_tokens[1], "X")) {
			inst->_opA._pvalue = &a->_zone._left;
		} else
		if (!scumm_stricmp(_tokens[1], "Y")) {
			inst->_opA._pvalue = &a->_zone._top;
		} else
		if (!scumm_stricmp(_tokens[1], "Z")) {
			inst->_opA._pvalue = &a->_z;
		} else
		if (!scumm_stricmp(_tokens[1], "F")) {
			inst->_opA._pvalue = &a->_frame;
		} else {
			inst->_flags |= kInstUsesLocal;
			inst->_opA = getLValue(inst, _tokens[1], locals, a);
		}

		inst->_opB = getLValue(inst, _tokens[2], locals, a);

		if (!scumm_stricmp(_tokens[3], "mod")) {
			inst->_flags |= kInstMod;
		}
		break;

	case INST_SET:	// set
		inst->_opA = getLValue(inst, _tokens[1], locals, a);
		inst->_flags |= kInstUsesLocal;
		inst->_opB = getLValue(inst, _tokens[2], locals, a);
		break;

	case INST_MOVE: // move
		inst->_opA = getLValue(inst, _tokens[1], locals, a);
		inst->_opB = getLValue(inst, _tokens[2], locals, a);
		break;

	case INST_PUT:	// put
		if (!scumm_stricmp(_tokens[1], a->_zone._label._text)) {
			inst->_opBase._a = a;
		} else {
			inst->_opBase._a = findAnimation(_tokens[1]);
		}

		inst->_opA = getLValue(inst, _tokens[2], locals, a);
		inst->_opB = getLValue(inst, _tokens[3], locals, a);
		if (!scumm_stricmp(_tokens[4], "masked")) {
			inst->_flags |= kInstMaskedPut;
		}
		break;

	case INST_CALL: {	// call
		int16 _ax = searchTable(_tokens[1], _callableNames);
		inst->_opBase._index = _ax - 1;
		if (_ax - 1 < 0) exit(0);
	}
	break;

	case INST_SOUND:	// sound
		inst->_opBase._z = findZone(_tokens[1]);
		break;

	case INST_ENDLOOP:	// endloop
	case INST_SHOW: // show
	case INST_WAIT: // wait
		break;

	default:	// local definition
		strcpy(_localNames[_numLocals], _tokens[0]);
		locals[_numLocals]._value = atoi(_tokens[2]);

		if (_tokens[3][0] != '\0') {
			locals[_numLocals]._min = atoi(_tokens[3]);
			locals[_numLocals]._max = atoi(_tokens[4]);
		} else {
			locals[_numLocals]._min = -10000;
			locals[_numLocals]._max = 10000;
		}

		inst->_opA._local = &locals[_numLocals];
		inst->_opB._value = locals[_numLocals]._value;

		inst->_flags = kInstUsesLiteral | kInstUsesLocal;
		inst->_index = INST_SET;
		_numLocals++;
		break;

	}


	return;
}

LValue getLValue(Instruction *inst, char *str, LocalVariable *locals, Animation *a) {

	LValue v;

	v._pvalue = 0;	// should stop compiler from complaining

	if (isdigit(str[0]) || str[0] == '-') {
		inst->_flags |= kInstUsesLiteral;
		v._value = atoi(str);
		return v;
	}

	for (uint16 _si = 0; _si < 10; _si++) {
		if (!scumm_stricmp(str, _localNames[_si])) {
			v._local = &locals[_si];
			return v;
		}
	}

	if (str[1] == '.') {
		a = _vm->findAnimation(&str[2]);
	}

	if (str[0] == 'X') {
		v._pvalue = &a->_zone._left;
	} else
	if (str[0] == 'Y') {
		v._pvalue = &a->_zone._top;
	} else
	if (str[0] == 'Z') {
		v._pvalue = &a->_z;
	} else
	if (str[0] == 'F') {
		v._pvalue = &a->_frame;
	}

	return v;
}



void jobRunScripts(void *parm, Job *j) {
	debugC(3, kDebugJobs, "jobRunScripts");

	static uint16 modCounter = 0;

	Animation *a = (Animation*)_vm->_animations._next;

	StaticCnv v18;
	WalkNode *v4 = NULL;

	if (a->_zone._flags & kFlagsCharacter) a->_z = a->_zone._top + a->height();
	for ( ; a; a = (Animation*)a->_zone._next) {

		if ((a->_zone._flags & kFlagsActing) == 0) continue;
		Instruction *inst = a->_program->_ip;

//		printf("Animation: %s, flags: %x\n", a->_zone._name, a->_zone._flags);

		while ((inst->_index != INST_SHOW) && (a->_zone._flags & kFlagsActing)) {

			debugC(1, kDebugJobs, "Animation: %s, instruction: %s", a->_zone._label._text, inst->_index == INST_END ? "end" : _instructionNames[inst->_index - 1]);

			switch (inst->_index) {
			case INST_ENDLOOP:	// endloop
				if (--a->_program->_loopCounter > 0) {
					inst = a->_program->_loopStart;
				}
				break;

			case INST_OFF:	{// off
				inst->_opBase._a->_zone._flags |= kFlagsRemove;
//				v1C = inst->_opBase;
				}
				break;

			case INST_ON:	// on
				inst->_opBase._a->_zone._flags |= kFlagsActive;
				inst->_opBase._a->_zone._flags &= ~kFlagsRemove;
				break;

			case INST_START:	// start
//				v1C = inst->_opBase;
				inst->_opBase._a->_zone._flags |= (kFlagsActing | kFlagsActive);
				break;

			case INST_LOOP: // loop
				if (inst->_flags & kInstUsesLiteral) {
					a->_program->_loopCounter = inst->_opBase._loopCounter._value;
				} else {
					a->_program->_loopCounter = *inst->_opBase._loopCounter._pvalue;
				}
				a->_program->_loopStart = inst;
				break;

			case INST_INC:	// inc
			case INST_DEC: {	// dec
				int16 _si = 0;
				int16 _ax = 0, _bx = 0;
				if (inst->_flags & kInstUsesLiteral) {
					_si = inst->_opB._value;
				} else {
					_si = *inst->_opB._pvalue;
				}
				if (inst->_flags & kInstMod) {	// mod
					_bx = (_si > 0 ? _si : -_si);
					if (modCounter % _bx != 0) break;

					_si = (_si > 0 ?  1 : -1);
				}
				if (inst->_flags & kInstUsesLocal) {	// local
					if (inst->_index == INST_INC) _ax = _si;
					else _ax = -_si;

					inst->_opA._local->_value += _ax;
					wrapLocalVar(inst->_opA._local);
					break;
				}

				// built-in variable (x, y, z, f)
				if (inst->_index == INST_INC) _ax = _si;
				else _ax = -_si;
				*inst->_opA._pvalue += _ax;
			}
			break;

			case INST_MOVE: // move
				v4 = buildWalkPath(*inst->_opA._pvalue, *inst->_opB._pvalue);
				_vm->addJob(&jobWalk, v4, kPriority19 );
				_engineFlags |= kEngineWalking;
				break;

			case INST_PUT:	// put
				v18._width = inst->_opBase._a->width();
				v18._height = inst->_opBase._a->height();
				v18._data0 = inst->_opBase._a->getFrameData(inst->_opBase._a->_frame);
				v18._data1 = NULL; // inst->_opBase._a->_cnv.field_8[inst->_opBase._a->_frame];

				if (inst->_flags & kInstMaskedPut) {
					uint16 _si = _vm->_gfx->queryMask(inst->_opB._value);
					_vm->_gfx->blitCnv(&v18, inst->_opA._value, inst->_opB._value, _si, Gfx::kBitBack);
					_vm->_gfx->blitCnv(&v18, inst->_opA._value, inst->_opB._value, _si, Gfx::kBit2);
				} else {
					_vm->_gfx->flatBlitCnv(&v18, inst->_opA._value, inst->_opB._value, Gfx::kBitBack);
					_vm->_gfx->flatBlitCnv(&v18, inst->_opA._value, inst->_opB._value, Gfx::kBit2);
				}
				break;

			case INST_END:	// exit
				if ((a->_zone._flags & kFlagsLooping) == 0) {
					a->_zone._flags &= ~kFlagsActing;
					runCommands(a->_zone._commands, (Zone*)&a->_zone);
				}
				a->_program->_ip = (Instruction*)a->_program->_next;
				goto label1;


			case INST_CALL: // call
				_callables[inst->_opBase._index](0);
				break;

			case INST_WAIT: // wait
				if (_engineFlags & kEngineWalking) goto label1;
				break;

			case INST_SOUND:	// sound
				_activeZone = inst->_opBase._z;
				break;

			default: {			// INST_SET, INST_X, INST_Y, INST_Z, INST_F
				int16 _si;
				if (inst->_flags & kInstUsesLiteral) {
					_si = inst->_opB._value;
				} else {
					_si = *inst->_opB._pvalue;
				}

				if (inst->_flags & kInstUsesLocal) {
					inst->_opA._local->_value = _si;
				} else {
					*inst->_opA._pvalue = _si;
				}
				}
				break;

			}

			inst = (Instruction*)inst->_next;
		}

		a->_program->_ip = (Instruction*)inst->_next;

label1:
		if (a->_zone._flags & kFlagsCharacter)
			a->_z = a->_zone._top + a->height();
	}

	_vm->sortAnimations();
	modCounter++;

	return;
}

void wrapLocalVar(LocalVariable *local) {
//	  printf("wrapLocalVar(v: %i, min: %i, max: %i)\n", local->_value, local->_min, local->_max);

	if (local->_value >= local->_max)
		local->_value = local->_min;
	if (local->_value < local->_min)
		local->_value = local->_max - 1;

	return;
}



void Parallaction::sortAnimations() {
	Node v14;
	memset(&v14, 0, sizeof(Node));

	_vm->_char._ani._z = _vm->_char._ani.height() + _vm->_char._ani._zone._top;

	Animation *vC = (Animation*)_animations._next;
	Node *v8;
	Animation *v4;

	while (vC) {

		v8 = &v14;

		while ((v8->_next != NULL) && (vC->_z >= ((Animation*)(v8->_next))->_z)) {
			v8 = v8->_next;
		}

		v4 = (Animation*)vC->_zone._next;

		addNode(v8, &vC->_zone);

		vC = v4;
	}

	memcpy(&_animations, &v14, sizeof(Node));

	_animations._next->_prev = &_animations;

	return;
}


} // namespace Parallaction
