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

#ifndef INPUT_TOKENIZER_H
#define INPUT_TOKENIZER_H

#include <fstream>
#include <Addr.h>

class InputTokenizer {
public:
	struct Lexeme {
		enum Type {
			TKN_INVALID_TOKEN = -2,
			TKN_UNEXPECTED_EOF,
			TKN_EOF,
			TKN_BRACKET_OPEN,
			TKN_BRACKET_CLOSE,
			TKN_ADDR,
			TKN_NUMBER,
			TKN_BOOL,
			TKN_STRING,
			TKN_KEYWORD
		};

		enum Type type;
		std::string token;

		union {
			Addr addr;
			int number;
			bool boolean;
		} data;

		Lexeme() : type(TKN_EOF), token("") {}
		virtual ~Lexeme() {}
	};

	InputTokenizer(std::fstream& input);
	virtual ~InputTokenizer();

	Lexeme nextToken();

private:
	std::fstream& m_input;

	int nextChar();

};

#endif
