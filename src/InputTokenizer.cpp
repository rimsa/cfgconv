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

#include <InputTokenizer.h>

InputTokenizer::InputTokenizer(std::fstream& input)
	: m_input(input) {
	m_input >> std::noskipws;
}

InputTokenizer::~InputTokenizer() {
}

InputTokenizer::Lexeme InputTokenizer::nextToken() {
	Lexeme lex;

	int state = 1;
	while (state != 8) {
		int c = this->nextChar();
		switch (state) {
			case 1:
				if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
					state = 1;
				} else if (c == '0') {
					lex.token += (char) c;
					lex.type = Lexeme::TKN_NUMBER;
					lex.data.number = 0;
					state = 2;
				} else if (c >= '1' && c <= '9') {
					lex.token += (char) c;
					lex.type = Lexeme::TKN_NUMBER;
					state = 4;
				} else if (std::isalpha(c)) {
					lex.token += (char) std::tolower(c);
					state = 5;
				} else if (c == '[') {
					lex.token += (char) c;
					lex.type = Lexeme::TKN_BRACKET_OPEN;
					state = 8;
				} else if (c == ']') {
					lex.token += (char) c;
					lex.type = Lexeme::TKN_BRACKET_CLOSE;
					state = 8;
				} else if (c == '\"' || c == '\'') {
					lex.type = Lexeme::TKN_STRING;
					state = 6;
				} else if (c == '#') {
					state = 7;
				} else if (c == -1) {
					lex.type = Lexeme::TKN_EOF;
					state = 8;
				} else {
					lex.type = Lexeme::TKN_INVALID_TOKEN;
					state = 8;
				}

				break;
			case 2:
				if (std::tolower(c) == 'x') {
					lex.token += (char) std::tolower(c);
					lex.type = Lexeme::TKN_ADDR;
					state = 3;
				} else {
					if (c != -1)
						m_input.putback(c);

					state = 8;
				}

				break;
			case 3:
				if (std::isdigit(c) ||
						(std::tolower(c) >= 'a' && std::tolower(c) <= 'f')) {
					lex.token += (char) std::tolower(c);
					state = 3;
				} else {
					lex.data.addr = std::stoul(lex.token.substr(2), 0, 16);

					if (c != -1)
						m_input.putback(c);

					state = 8;
				}

				break;
			case 4:
				if (std::isdigit(c)) {
					lex.token += (char) c;
					state = 4;
				} else {
					lex.data.number = std::stoi(lex.token);

					if (c != -1)
						m_input.putback(c);

					state = 8;
				}

				break;
			case 5:
				if (std::isalpha(c)) {
					lex.token += (char) std::tolower(c);
					state = 5;
				} else if (c == '-') {
					lex.token += (char) c;
					state = 5;
				} else {
					if (lex.token == "true") {
						lex.type = Lexeme::TKN_BOOL;
						lex.data.boolean = true;
					} else if (lex.token == "false") {
						lex.type = Lexeme::TKN_BOOL;
						lex.data.boolean = false;
					} else {
						lex.type = Lexeme::TKN_KEYWORD;
					}

					if (c != -1)
						m_input.putback(c);

					state = 8;
				}

				break;
			case 6:
				if (c == -1) {
					lex.type = Lexeme::TKN_UNEXPECTED_EOF;
					state = 8;
				} else {
					if (c == '\"' || c == '\'')
						state = 8;
					else {
						lex.token += (char) c;
						state = 6;
					}
				}

				break;
			case 7:
				if (c == -1) {
					state = 8;
				} else {
					if (c == '\n')
						state = 1;
					else
						state = 7;
				}

				break;
			default:
				lex.type = Lexeme::TKN_INVALID_TOKEN;
				state = 8;
				break;
		}
	}

	return lex;
}

int InputTokenizer::nextChar() {
	if (m_input.eof())
		return -1;

	char ch;
	m_input >> ch;
	return ch;
}