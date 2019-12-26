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
#include <algorithm>

#include <CFG.h>
#include <CfgNode.h>
#include <BFTraceReader.h>

BFTraceReader::BFTraceReader(const std::string& filename)
	: CFGReader(filename), m_tokens(m_input), m_current(m_tokens.nextToken()) {
}

BFTraceReader::~BFTraceReader() {
}

void BFTraceReader::loadCFGs() {
	Symbol* sym = 0;
	std::list<Symbol*> symbols;

	while (m_current.type == InputTokenizer::Lexeme::TKN_KEYWORD) {
		std::string keyword = m_current.token;
		matchToken(InputTokenizer::Lexeme::TKN_KEYWORD);

		if (keyword == "symbol") {
			sym = new Symbol();
			symbols.push_back(sym);

			sym->start = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			sym->end = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			sym->filename = m_current.token;
			matchToken(InputTokenizer::Lexeme::TKN_STRING);

			sym->functname = m_current.token;
			matchToken(InputTokenizer::Lexeme::TKN_STRING);

			sym->bias = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
		} else if (keyword == "program-entry") {
			sym = 0;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
		} else if (keyword == "block") {
			assert(sym != 0);

			Addr faddr = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
			assert(faddr == sym->start);

			Addr bb_addr = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
			BasicBlock& bb = sym->blocks[bb_addr];

			Addr bb_end = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
			bb.size = bb_end - bb_addr;

			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			std::string term_type = m_current.token;
			matchToken(InputTokenizer::Lexeme::TKN_KEYWORD);
			if (term_type == "jump")
				bb.type = BFTraceReader::JUMP;
			else if (term_type == "call")
				bb.type = BFTraceReader::CALL;
			else if (term_type == "return")
				bb.type = BFTraceReader::RETURN;
			else {
				assert(term_type == "other");
				bb.type = BFTraceReader::OTHER;
			}

			bool is_entry = m_current.data.boolean;
			matchToken(InputTokenizer::Lexeme::TKN_BOOL);
			if (is_entry)
				sym->entries.insert(bb_addr);

			bool is_exit = m_current.data.boolean;
			matchToken(InputTokenizer::Lexeme::TKN_BOOL);
			bb.is_exit = is_exit;
		} else if (keyword == "call") {
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
		} else if (keyword == "return") {
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);
		} else if (keyword == "br") {
			assert(sym != 0);

			Addr src = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			Addr dst = m_current.data.addr;
			matchToken(InputTokenizer::Lexeme::TKN_ADDR);

			sym->edges[src].insert(dst);
		} else {
			assert(false);
		}
	}

	matchToken(InputTokenizer::Lexeme::TKN_EOF);

	for (Symbol* sym : symbols) {
		for (Addr entry : sym->entries) {
			CFG* cfg = this->instance(entry);
			cfg->setFunctionName(sym->filename + "::" + sym->functname);

			std::list<Addr> nodes;
			nodes.push_back(entry);
			for (std::list<Addr>::iterator it = nodes.begin(), ed = nodes.end();
					it != ed; ++it) {
				Addr addr = *it;
				assert(addr != 0);

				assert(sym->blocks.find(addr) != sym->blocks.end());
				BasicBlock& bb = sym->blocks[addr];

				CfgNode* node = cfg->nodeByAddr(addr);
				if (node == 0) {
					node = new CfgNode(CfgNode::CFG_BLOCK);
					node->setData(new CfgNode::BlockData(addr, bb.size));
					cfg->addNode(node);
				} else {
					assert(node->type() == CfgNode::CFG_PHANTOM);
					node->setData(new CfgNode::BlockData(addr, bb.size));
				}

				if (addr == entry) {
					assert(cfg->entryNode() == 0);
					CfgNode* entry = CFGReader::entryNode(cfg);
					cfg->addEdge(entry, node);
				}

				if (bb.is_exit || bb.type == BFTraceReader::RETURN) {
					CfgNode* exit = CFGReader::exitNode(cfg);
					cfg->addEdge(node, exit);
				}

				for (Addr dst : sym->edges[addr]) {
					cfg->addEdge(node, CFGReader::nodeWithAddr(cfg, dst));

					if (std::find(nodes.begin(), ed, dst) == ed)
						nodes.push_back(dst);
				}
			}

			cfg->check();
		}

		delete sym;
	}
}

void BFTraceReader::matchToken(InputTokenizer::Lexeme::Type type) {
	assert(m_current.type == type);
	m_current = m_tokens.nextToken();
}
