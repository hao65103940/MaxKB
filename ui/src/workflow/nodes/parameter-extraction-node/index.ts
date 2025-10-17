import ParameterExtractionNodeVue from './index.vue'
import { AppNode, AppNodeModel } from '@/workflow/common/app-node'

class ParameterExtractionNode extends AppNode {
  constructor(props: any) {
    super(props, ParameterExtractionNodeVue)
  }
}

export default {
  type: 'parameter-extraction-node',
  model: AppNodeModel,
  view: ParameterExtractionNode,
}
