#coding=utf-8
"""
    @project: MaxKB
    @Author：虎²
    @file： base_variable_aggregation_node.py
    @date：2025/10/23 17:42
    @desc:
"""
from application.flow.i_step_node import NodeResult
from application.flow.step_node.variable_aggregation_node.i_variable_aggregation_node import IVariableAggregation


class BaseVariableAggregationNode(IVariableAggregation):

    def save_context(self, details, workflow_manage):
        for key, value in details.get('result').items():
            self.context['key'] = value
        self.context['result'] = details.get('result')

    def get_first_non_null(self, variable_list):

        for variable in variable_list:
            v = self.workflow_manage.get_reference_field(
                variable.get('variable')[0],
                variable.get('variable')[1:])
            if v is not None:
                return v
        return None

    def set_variable_to_json(self, variable_list):

        return {variable.get('variable')[1:][0]: self.workflow_manage.get_reference_field(
                variable.get('variable')[0],
                variable.get('variable')[1:]) for variable in variable_list}

    def execute(self,strategy,group_list,**kwargs) -> NodeResult:
        strategy_map = {'first_non_null':self.get_first_non_null,
                        'variable_to_json': self.set_variable_to_json,
                        }

        result = { item.get('group_name'):strategy_map[strategy](item.get('variable_list'))  for item in group_list}

        return NodeResult({'result': result,**result},{})

    def get_details(self, index: int, **kwargs):
        return {
            'name': self.node.properties.get('stepName'),
            "index": index,
            'run_time': self.context.get('run_time'),
            'type': self.node.type,
            'result': self.context.get('result'),
            'status': self.status,
            'err_message': self.err_message
        }