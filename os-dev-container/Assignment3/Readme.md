# Assignment Overview

In this assignment, the goal is to write a simple memory management simulator in C that supports paging. In this setup, the logical address space ($2^{16}=65,536$ bytes) is larger than the physical address space ($2^{15}$ bytes), and the page size is 256 bytes. The maximum number of entries in the TLB = 16.