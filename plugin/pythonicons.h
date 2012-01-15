/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>
   
   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PYQTC_PYTHONICONS_H
#define PYQTC_PYTHONICONS_H

#include "rpc.pb.h"

#include <QIcon>
#include <QScopedPointer>


namespace CPlusPlus {
class Icons;
}


namespace pyqtc {

class PythonIcons {
public:
  PythonIcons();

  QIcon IconForCompletionProposal(const pb::CompletionResponse_Proposal& proposal) const;
  QIcon IconForSearchResult(const pb::SearchResponse_Result& result) const;

private:
  QScopedPointer<CPlusPlus::Icons> cpp_icons_;
};

} // namespace pyqtc

#endif // PYQTC_PYTHONICONS_H
