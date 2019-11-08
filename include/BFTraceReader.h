/*

   Copyright (C) 2019, Andrei Rimsa (andrei@cefetmg.br)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.

*/

#ifndef BFTRACE_READER_H
#define BFTRACE_READER_H

#include <map>
#include <set>
#include <list>

#include <CFGReader.h>
#include <InputTokenizer.h>

class BFTraceReader : public CFGReader {
public:
	enum TerminatorType {
		JUMP,
		CALL,
		RETURN,
		OTHER
	};

	struct BasicBlock {
		int size;
		TerminatorType type;
		bool is_exit;
	};

	struct Symbol {
		Addr start;
		Addr end;
		std::string filename;
		std::string functname;
		Addr bias;

		std::map<Addr, BasicBlock> blocks;
		std::map<Addr, std::set<Addr>> edges;
		std::set<Addr> entries;
	};

	BFTraceReader(const std::string& filename);
	virtual ~BFTraceReader();

	virtual void loadCFGs();

private:
	InputTokenizer m_tokens;
	InputTokenizer::Lexeme m_current;

	void matchToken(InputTokenizer::Lexeme::Type type);

};

#endif
