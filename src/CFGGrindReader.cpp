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

#include <cassert>
#include <CFG.h>
#include <CfgNode.h>
#include <CFGGrindReader.h>

CFGGrindReader::CFGGrindReader(const std::string& filename)
	: CFGReader(filename), m_tokens(m_input), m_current(m_tokens.nextToken()) {
}

CFGGrindReader::~CFGGrindReader() {
}

#include <iostream>

void CFGGrindReader::loadCFGs() {
	while (m_current.type == InputTokenizer::Lexeme::TKN_BRACKET_OPEN) {
		matchToken(InputTokenizer::Lexeme::TKN_BRACKET_OPEN);

		std::string keyword = m_current.token;
		matchToken(InputTokenizer::Lexeme::TKN_KEYWORD);

		if (keyword == "cfg") {
			Addr addr = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			unsigned long long execs = 0;
			if (m_current.type == InputTokenizer::Lexeme::TKN_COLON) {
				matchToken(InputTokenizer::Lexeme::TKN_COLON);

				execs = m_current.data.number;
				matchToken(InputTokenizer::Lexeme::TKN_NUMBER);
			}

			std::string fname = m_current.token;
			matchToken(InputTokenizer::Lexeme::TKN_STRING);

			matchToken(InputTokenizer::Lexeme::TKN_BOOL);

			CFG* cfg = this->instance(addr);
			cfg->setFunctionName(fname);
			cfg->updateExecs(execs);
		} else if (keyword == "node") {
			Addr faddr = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			CFG* cfg = this->instance(faddr);

			Addr baddr = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			CfgNode* node = cfg->nodeByAddr(baddr);
			CfgNode::BlockData* data = new CfgNode::BlockData(baddr);
			if (node == 0) {
				node = new CfgNode(CfgNode::CFG_BLOCK);
				node->setData(data);
				cfg->addNode(node);
			} else {
				assert(node->type() == CfgNode::CFG_PHANTOM);
				node->setData(data);
			}

			if (baddr == cfg->addr()) {
				assert(cfg->entryNode() == 0);
				CfgNode* entry = CFGReader::entryNode(cfg);
				cfg->addEdge(entry, node, cfg->execs());
			}

			int bsize = m_current.data.number;
			matchToken(InputTokenizer::Lexeme::TKN_NUMBER);

			matchToken(InputTokenizer::Lexeme::TKN_BRACKET_OPEN);
			Addr iaddr = baddr;
			while (m_current.type != InputTokenizer::Lexeme::TKN_BRACKET_CLOSE) {
				int size = m_current.data.number;
				matchToken(InputTokenizer::Lexeme::TKN_NUMBER);

				Instruction* instr = Instruction::get(iaddr, size);
				data->addInstruction(instr);

				iaddr += size;
			}
			matchToken(InputTokenizer::Lexeme::TKN_BRACKET_CLOSE);

			assert(bsize == data->size());

			matchToken(InputTokenizer::Lexeme::TKN_BRACKET_OPEN);
			while (m_current.type != InputTokenizer::Lexeme::TKN_BRACKET_CLOSE) {
				Addr caddr = m_current.data.addr;
				matchToken(InputTokenizer::Lexeme::TKN_ADDR);

				CFG* call = this->instance(caddr);
				data->addCall(call);
			}
			matchToken(InputTokenizer::Lexeme::TKN_BRACKET_CLOSE);

			bool indirect = m_current.data.boolean;
			matchToken(InputTokenizer::Lexeme::TKN_BOOL);
			data->setIndirect(indirect);

			matchToken(InputTokenizer::Lexeme::TKN_BRACKET_OPEN);
			while (m_current.type != InputTokenizer::Lexeme::TKN_BRACKET_CLOSE) {
				CfgNode* dst = 0;

				switch (m_current.type) {
					case InputTokenizer::Lexeme::TKN_ADDR:
						{
							Addr saddr = m_current.data.addr;
							matchToken(InputTokenizer::Lexeme::TKN_ADDR);

							dst = this->nodeWithAddr(cfg, saddr);
						}

						break;
					case InputTokenizer::Lexeme::TKN_KEYWORD:
						keyword = m_current.token;
						matchToken(InputTokenizer::Lexeme::TKN_KEYWORD);

						if (keyword == "exit")
							dst = CFGReader::exitNode(cfg);
						else if (keyword == "halt")
							dst = CFGReader::haltNode(cfg);
						else {
							std::cout << keyword << std::endl;
							assert(false);
						}

						break;
					default:
						assert(false);
				}

				unsigned long long count = 0;
				if (m_current.type == InputTokenizer::Lexeme::TKN_COLON) {
					matchToken(InputTokenizer::Lexeme::TKN_COLON);

					count = m_current.data.number;
					matchToken(InputTokenizer::Lexeme::TKN_NUMBER);
				}

				cfg->addEdge(node, dst, count);
			}
			matchToken(InputTokenizer::Lexeme::TKN_BRACKET_CLOSE);
		} else {
			assert(false);
		}

		matchToken(InputTokenizer::Lexeme::TKN_BRACKET_CLOSE);
	}

	matchToken(InputTokenizer::Lexeme::TKN_EOF);

	for (CFG* cfg : this->cfgs())
		cfg->check();
}

void CFGGrindReader::matchToken(InputTokenizer::Lexeme::Type type) {
	assert(m_current.type == type);
	m_current = m_tokens.nextToken();
}
