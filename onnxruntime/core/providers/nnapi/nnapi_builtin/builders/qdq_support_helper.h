// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <string>
#include "core/graph/basic_types.h"
#include "core/providers/nnapi/nnapi_builtin/nnapi_lib/NeuralNetworksTypes.h"
#include "core/optimizer/qdq_transformer/selectors_actions/qdq_selectors.h"
#include "core/optimizer/selectors_actions/helpers.h"

namespace onnxruntime {

class GraphViewer;
class Node;

namespace nnapi {

struct Selector {
  using OpVersionsMap = std::unordered_map<std::string, std::vector<ONNX_NAMESPACE::OperatorSetVersion>>;

  Selector(const OpVersionsMap& ops_and_versions_in,
           std::unique_ptr<QDQ::BaseSelector> selector_in)
      : op_versions_map{ops_and_versions_in},
        selector{std::move(selector_in)} {}

  OpVersionsMap op_versions_map;
  std::unique_ptr<QDQ::BaseSelector> selector;

  ORT_DISALLOW_COPY_AND_ASSIGNMENT(Selector);
};

class Selectors {
 public:
  Selectors() = default;

  Selectors(Selectors&& rhs) noexcept
      : selectors_set_{std::move(rhs.selectors_set_)} {}

  void RegisterSelector(const Selector::OpVersionsMap& ops_and_versions_in,
                        std::unique_ptr<QDQ::BaseSelector> selector_in);

  const std::unordered_set<std::unique_ptr<Selector>>& SelectorsSet() const {
    return selectors_set_;
  }

  ORT_DISALLOW_COPY_AND_ASSIGNMENT(Selectors);

  std::unordered_set<std::unique_ptr<Selector>> selectors_set_;
};

class QDQSupportHelper {
 public:
  QDQSupportHelper(Selectors&& selectors);

  bool IsNodeInQDQGroup(const Node& node) const;

  void GetQDQNodeGroup(const onnxruntime::GraphViewer& graph_viewer, const Node& node);

  void GetQDQNodeGroups(const onnxruntime::GraphViewer& graph_viewer);

  std::unordered_map<const Node*, QDQ::NodeGroupNonIndex> target_node_to_qdq_group_;

 private:
  std::optional<QDQ::NodeGroup> Match(const GraphViewer& graph_viewer, const Node& node) const;

  Selectors selectors_;

  std::unordered_map<std::string, const Selector*> op_type_to_selectors_map_;

  std::unordered_set<const Node*> nodes_in_qdq_group;
};

/* Selector Rules Related */
void ConvQDQRules(Selectors& qdq_selectors);

Selectors CreateSelectors();

}  // namespace nnapi
}  // namespace onnxruntime