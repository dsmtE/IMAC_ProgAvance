////-----------------------------------------------------------------------------------
////
//// Paratrooper
//// Copyright (C) 2007-2008  Sukender
//// For more information, contact us : sukender@free.fr
////
//// This program is free software; you can redistribute it and/or modify
//// it under the terms of the GNU General Public License as published by
//// the Free Software Foundation; either version 3 of the License, or
//// (at your option) any later version.
////
//// For any use that is not compatible with the terms of the GNU 
//// General Public License, please contact the authors for alternative
//// licensing options.
////
//// This program is distributed in the hope that it will be useful,
//// but WITHOUT ANY WARRANTY; without even the implied warranty of
//// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// GNU General Public License for more details.
////
//// You should have received a copy of the GNU General Public License along
//// with this program; if not, write to the Free Software Foundation, Inc.,
//// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
////
////-----------------------------------------------------------------------------------
//
//#include "HiScores.h"
//
//#include <fstream>
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>
//#include <PVLE/Util/Util.h>
//#include <limits.h>
//
//void HiScores::reset() {
//	//memset(scores, 0, Game::NB_DIFFICULTY_LEVELS * MAX_SCORES);
//	std::fill(scores, scores+MAX_SCORES, 0);
//	std::fill(names,  names +MAX_SCORES, "---");
//}
//
//void HiScores::save(const boost::filesystem::path & path) const {
//	std::ofstream ofs(path.native_file_string().c_str());
//	boost::archive::text_oarchive oarchive(ofs);
//	oarchive << *this;
//}
//
//void HiScores::load(const boost::filesystem::path & path) {
//	std::ifstream ifs(path.native_file_string().c_str(), std::ios::binary);
//	boost::archive::text_iarchive iarchive(ifs);
//	iarchive >> *this;
//}
//
//void HiScores::add(const std::string & name, UINT score) {
//	if (!isHiScore(score)) return;
//	//if (name.size()>MAX_LENGTH) TODO
//
//	UINT prevScore = UINT_MAX;
//	for(int i=0; i<MAX_SCORES; ++i) {
//		if (score <= prevScore && score > scores[i]) {
//			// Insert
//			for(int j=MAX_SCORES-2; j>=i; --j) {
//				names[j+1]  = names[j];
//				scores[j+1] = scores[j];
//			}
//			names[i]  = name;
//			scores[i] = score;
//			return;
//		}
//		prevScore = scores[i];
//	}
//}
//
////void HiScores::add(const std::string & name) { add(); }
////void HiScores::isHiScore(const std::string & name) {}
//
//void HiScores::merge(const HiScores & v) {
//	for(int i=0; i<MAX_SCORES; ++i) {
//		add(v.names[i], v.scores[i]);
//	}
//}
