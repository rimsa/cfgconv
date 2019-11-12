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

#ifndef CFGGRIND_READER_H
#define CFGGRIND_READER_H

#include <CFGReader.h>
#include <InputTokenizer.h>

class CFGGrindReader : public CFGReader {
public:
	CFGGrindReader(const std::string& filename);
	virtual ~CFGGrindReader();

	virtual void loadCFGs();

private:
	InputTokenizer m_tokens;
	InputTokenizer::Lexeme m_current;

	void matchToken(InputTokenizer::Lexeme::Type type);

};

#endif
