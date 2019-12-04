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

#ifndef CFGEDGE_H
#define CFGEDGE_H

class CfgNode;

class CfgEdge {
public:
	CfgEdge(CfgNode* src, CfgNode* dst, unsigned long long count = 0);
	virtual ~CfgEdge() {}

	CfgNode* source() const { return m_src; }
	CfgNode* destination() const { return m_dst; }
	unsigned long long count() const { return m_count; }

	void setCount(unsigned long long count) { m_count = count; }
	void updateCount(unsigned long long count) { m_count += count; }

private:
	CfgNode* m_src;
	CfgNode* m_dst;
	unsigned long long m_count;

};

#endif