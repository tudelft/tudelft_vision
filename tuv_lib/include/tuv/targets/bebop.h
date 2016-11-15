/*
 * This file is part of the XXX distribution (https://github.com/xxxx or http://xxx.github.io).
 * Copyright (c) 2016 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TARGETS_BEBOP_H_
#define TARGETS_BEBOP_H_

#include <tuv/targets/linux.h>
#include <stdint.h>
#include <map>

/**
 * @brief Bebop and Bebop 2
 *
 * This is a specific target implementation for the Bebop and the Bebop 2.
 */
class Bebop : public Linux {
  private:
    static int pagemap_fd;                          ///< The memory pagemap file pointer
    static std::map<uintptr_t, uint64_t> mem_map;   ///< Virtual to physical address mapping cache (Only contigious ones)

    void openPagemap(void);
    void closePagemap(void);

  public:
    Bebop(void);
    ~Bebop(void);

    Cam::Ptr getCamera(uint32_t id);

    /* Usefull functtions */
    static void virt2phys(uintptr_t vaddr, uint64_t *paddr);
    static bool checkContiguity(uintptr_t vaddr, uint64_t size, uint64_t *paddr, bool cache = false);
};

#endif /* TARGETS_BEBOP_H_ */
