import VariableSplittingNodeVue from './index.vue'
import { AppNode, AppNodeModel } from '@/workflow/common/app-node'

class VariableSplittingNode extends AppNode {
  constructor(props: any) {
    super(props, VariableSplittingNodeVue)
  }
}

export default {
  type: 'variable-splitting-node',
  model: AppNodeModel,
  view: VariableSplittingNode,
}
