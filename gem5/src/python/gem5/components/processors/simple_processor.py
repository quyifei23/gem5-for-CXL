# Copyright (c) 2021 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


from m5.util import warn
from .base_cpu_processor import BaseCPUProcessor
from ..processors.simple_core import SimpleCore

from .cpu_types import CPUTypes
from ...isas import ISA

from typing import Optional


class SimpleProcessor(BaseCPUProcessor):
    """
    A SimpleProcessor contains a number of cores of SimpleCore objects of the
    same CPUType.
    """

    def __init__(
        self, cpu_type: CPUTypes, num_cores: int, isa: Optional[ISA] = None
    ) -> None:
        """
        :param cpu_type: The CPU type for each type in the processor.
        :param num_cores: The number of CPU cores in the processor.

        :param isa: The ISA of the processor. This argument is optional. If not
        set the `runtime.get_runtime_isa` is used to determine the ISA at
        runtime. **WARNING**: This functionality is deprecated. It is
        recommended you explicitly set your ISA via SimpleProcessor
        construction.
        """
        if not isa:
            warn(
                "An ISA for the SimpleProcessor was not set. This will "
                "result in usage of `runtime.get_runtime_isa` to obtain the "
                "ISA. This function is deprecated and will be removed in "
                "future releases of gem5. Please explicitly state the ISA "
                "via the processor constructor."
            )
        super().__init__(
            cores=[
                SimpleCore(cpu_type=cpu_type, core_id=i, isa=isa)
                for i in range(num_cores)
            ]
        )
