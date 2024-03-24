# Copyright 2024 Utarn Technology Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import torch
import math
from .BaseFunction import SurrogateFunctionBase
from .BaseFunction import heaviside

def atan_backward(grad_output: torch.Tensor, x: torch.Tensor, alpha: float):
    return alpha / 2 / (1 + (math.pi / 2 * alpha * x).pow_(2)) * grad_output, None


class atan(torch.autograd.Function):
    @staticmethod
    def forward(ctx, x, alpha):
        if x.requires_grad:
            ctx.save_for_backward(x)
            ctx.alpha = alpha
        return heaviside(x)

    @staticmethod
    def backward(ctx, grad_output):
        return atan_backward(grad_output, ctx.saved_tensors[0], ctx.alpha)


class ATan(SurrogateFunctionBase):
    def __init__(self, alpha=2.0, spiking=True):
        '''
        * :ref:`中文API <ATan.__init__-cn>`
        .. _ATan.__init__-cn:

        :param alpha: 控制梯度平滑程度的参数
        :param spiking: 输出脉冲，默认为 ``True``，在前向传播时使用 ``heaviside`` 而在反向传播使用替代梯度。若为 ``False``
            则不使用替代梯度，前向传播时，使用反向传播时的梯度替代函数对应的原函数

        反向传播时使用反正切函数arc tangent的梯度替代函数

        * :ref:`API in English <ATan.__init__-en>`
        .. _ATan.__init__-en:

        :param alpha: parameter to control smoothness of gradient
        :param spiking: output spikes. The default is ``True`` which means that using ``heaviside`` in forward
            propagation and using surrogate gradient in backward propagation. If ``False``, in forward propagation,
            using the primitive function of the surrogate gradient function used in backward propagation

        The arc tangent surrogate spiking function

        '''
        super().__init__(alpha, spiking)

    @staticmethod

    def spiking_function(x, alpha):
        return atan.apply(x, alpha)

    @staticmethod

    def primitive_function(x: torch.Tensor, alpha: float):
        return (math.pi / 2 * alpha * x).atan_() / math.pi + 0.5

    @staticmethod

    def backward(grad_output, x, alpha):
        return atan_backward(grad_output, x, alpha)[0]

