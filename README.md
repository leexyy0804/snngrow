# SNNGrow

<p align="center">
  	<img alt="SNNGrow" src="./docs/en/source/_static/logo.png" width=50%>
</p>

English | [中文(Chinese)](./README_zh_CN.md) 

SNNGrow is a low-power, large-scale spiking neural network training and inference framework. SNNGrow does not rely on specialized hardware to implement an energy-efficient spiking computation mode. Using Cutlass, SNNGrow develops fundamental operations for spiking data (such as GEMM), replacing high-power-consuming multiply-add(MAD) operations with low-power addition(ADD) operations. Additionally, SNNGrow further reduces storage and bandwidth costs by utilizing the binary nature of spikes, resulting in several times speedup and storage savings. It preserves minimal energy cosumption while providing the superior learning abilities of large spiking neural network.

The vision of SNNGrow is to decode human intelligence and the mechanisms of its evolution, and to provide support for the development of brain-inspired intelligent agents in a future society where humans coexist with artificial intelligence.

- **[Documentation](https://snngrow.readthedocs.io/)**
- **[Source](https://github.com/snngrow/snngrow/)**

## Install

SNNGrow offers two installation methods.
Running the following command in your terminal will install the project:
### Install from Local(Recommended):
1. Install Pytorch
2. Install CUDA locally, make sure the CUDA version is consistent with the Pytorch CUDA version
3.  Download or clone SNNGrow from github
```
git clone https://github.com/snngrow/snngrow.git
```
4.  Enter the folder of SNNGrow and install braincog locally with setuptools
```
cd snngrow
python setup.py install
```
### Install Lite Version from PyPI(The Lite version does not support spiking computation mode):

```
pip install snngrow
```

## Quickstart

The code style of SNNGrow is consistent with Pytorch, allowing you to build spiking neural networks with simple code:
```
from snngrow.base import utils
from snngrow.base.neuron import LIFNode
import torch

x = torch.randn(2, 3, 5, 5)

net = torch.nn.Sequential(
    nn.Conv2d(1, 32, kernel_size=3),
    LIFNode(),
    nn.Flatten(),
    nn.Linear(54, 1)
)

y = net(x)
utils.reset(net)
```
An example of building a network using spiking computation mode:
```
import torch
import torch.nn as nn
from snngrow.base.neuron.LIFNode import LIFNode
from snngrow.base.surrogate import Sigmoid
import snngrow.base.nn as snngrow_nn
class SimpleNet(nn.Module):
    def __init__(self, T):
        super(SimpleNet, self).__init__()
        self.T = T
        self.surrogate = Sigmoid.Sigmoid(spike_out=True)
        self.classifier = nn.Sequential(
            nn.Flatten(),
            nn.Linear(28 * 28, 512),
            LIFNode(T=T, spike_out=True, surrogate_function=self.surrogate),
            snngrow_nn.Linear(512, 512, spike_in=True),
            LIFNode(T=T, spike_out=True, surrogate_function=self.surrogate),
            snngrow_nn.Linear(512, 128, spike_in=True),
            nn.Linear(128, 10)
        )
```

## Development plans

SNNGrow is still under active development:
- [x] Large-scale deep spiking neural network training and inference
- [x] Ultra-low energy consumption sparse spiking neural network computing
- [ ] Brain-inspired learning algorithm support
- [ ] Bionic neural network sparse structure support

## Cite

If you are using SNNGrow, please consider citing it as follows:
```
@misc{SNNGrow,
    title = {SNNGrow},
    author = {Lei, Yunlin and Gao, Lanyu and Yang, Xu and other contributors},
    year = {2024},
    publisher = {GitHub},
    journal = {GitHub repository},
    howpublished = {\url{https://github.com/snngrow/snngrow}},
}
```

## About

[Utarn Technology Co., Ltd.](https://www.utarn.com/w/home)and [Beijing Institute of Technology AETAS Laboratory](https://www.aetasbit.com/) are the main developers of SNNGrow.
