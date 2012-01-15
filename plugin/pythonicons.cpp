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

#include "pythonicons.h"

#include <cplusplus/Icons.h>

using namespace pyqtc;

PythonIcons::PythonIcons()
  : cpp_icons_(new CPlusPlus::Icons)
{
}

QIcon PythonIcons::IconForCompletionProposal(
      const pb::CompletionResponse_Proposal& proposal) const {
  CPlusPlus::Icons::IconType type = CPlusPlus::Icons::UnknownIconType;

  const bool is_private = proposal.name().startsWith("_");

  // Keywords are treated differently
  switch (proposal.scope()) {
  case pb::CompletionResponse_Proposal_Scope_KEYWORD:
    type = CPlusPlus::Icons::KeywordIconType;
    break;

  default:
    // Continue to look at the type
    switch (proposal.type()) {
    case pb::CompletionResponse_Proposal_Type_INSTANCE:
      type = is_private ?
            CPlusPlus::Icons::VarPrivateIconType :
            CPlusPlus::Icons::VarPublicIconType;
      break;

    case pb::CompletionResponse_Proposal_Type_CLASS:
      type = CPlusPlus::Icons::ClassIconType;
      break;

    case pb::CompletionResponse_Proposal_Type_FUNCTION:
      type = is_private ?
            CPlusPlus::Icons::FuncPrivateIconType :
            CPlusPlus::Icons::FuncPublicIconType;
      break;

    case pb::CompletionResponse_Proposal_Type_MODULE:
      type = CPlusPlus::Icons::NamespaceIconType;
      break;
    }
    break;
  }

  return cpp_icons_->iconForType(type);
}

QIcon PythonIcons::IconForSearchResult(const pb::SearchResponse_Result& result) const {
  CPlusPlus::Icons::IconType type = CPlusPlus::Icons::UnknownIconType;

  const bool is_private = result.symbol_name().startsWith("_");

  switch (result.symbol_type()) {
  case pb::VARIABLE:
    type = is_private ?
          CPlusPlus::Icons::VarPrivateIconType :
          CPlusPlus::Icons::VarPublicIconType;
    break;

  case pb::CLASS:
    type = CPlusPlus::Icons::ClassIconType;
    break;

  case pb::FUNCTION:
    type = is_private ?
          CPlusPlus::Icons::FuncPrivateIconType :
          CPlusPlus::Icons::FuncPublicIconType;
    break;

  case pb::MODULE:
    type = CPlusPlus::Icons::NamespaceIconType;
    break;
  }

  return cpp_icons_->iconForType(type);
}
