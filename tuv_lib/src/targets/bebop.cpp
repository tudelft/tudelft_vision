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

#include "targets/bebop.h"

#include "drivers/clogger.h"
#include "cam/cam_bebop.h"
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>


#define BIT(i)          ((uint64_t) 1 << i)             ///< Sets a bit (2^i)
#define PAGE_SHIFT      12                              ///< How much bits contain the memory page number
#define PAGE_SIZE       (1<<PAGE_SHIFT)                 ///< Size of the PAGE
#define PAGE_MASK       (((uint64_t)1<< PAGE_SHIFT) -1) ///< Mask to retreive the page number

#define PAGE_PRESENT    BIT(63)                         ///< If the page is present or not
#define PAGE_SWAPPED    BIT(62)                         ///< If the page is swapped
#define PAGE_PFN_MASK   (((uint64_t)1<< 55) -1)         ///< Mask for the page number

/**
 * @brief Initialize the Bebop platform
 *
 * This will open the pagemap for memory mapping to physical addresses.
 */
Bebop::Bebop(void) {
    CLogger::addAllOutput(&std::cout);
    openPagemap();
}

/**
 * @brief Close the Bebop platform
 *
 * This will close the pagemap for the memory mapping.
 */
Bebop::~Bebop(void) {
    closePagemap();
}

/**
 * @brief Get a specific camera
 *
 * This will setup the specific camera for each ID if not already initialized. It will first
 * search the camera list and if not found initialize a new camera.
 * The ID's are as following:
 * - 1: The front camera
 * - Other: Linux camera's
 * @param id The ID of the camera
 * @return The pointer to the camera
 */
Cam::Ptr Bebop::getCamera(uint32_t id) {
    /* First try to find if the camera already exists */
    Cam::Ptr cam = Target::getCamera(id);

    /* Create the bebop camera if possible */
    if(cam == nullptr) {
        switch(id) {
        // Bebop front camera
        case 1:
            cam = std::make_shared<CamBebop>();
            cams.push_back(std::make_pair(id, cam));
            break;

        // Create Linux camera by default
        default:
            cam = std::make_shared<CamLinux>("/dev/video" + std::to_string(id));
            break;
        }
    }

    return cam;
}

/**
 * @brief Open the pagemap
 *
 * This will open the pagemap file based on the current pid(process id).
 */
void Bebop::openPagemap(void) {
    std::string filename = "/proc/" + std::to_string(getpid()) + "/pagemap";

    pagemap_fd = open(filename.c_str(), O_RDONLY);
    if(pagemap_fd < 0) {
        throw std::runtime_error("Can't open pagemap for current process");
    }
}

/**
 * @brief Close the pagemap
 *
 * This will close the pagemap file
 */
void Bebop::closePagemap(void) {
    close(pagemap_fd);
}

/**
 * @brief Convert virtual to physical address
 *
 * This will convert a virtual memory address to a physical address based on the pagemap file
 * from the current process.
 * @param[in] vaddr The virtual address we want the physical address for
 * @param[out] paddr The physical address of the virtual address
 */
void Bebop::virt2phys(uint64_t vaddr, uint64_t *paddr) {
    // First seek to the correct page
    uint64_t index = (vaddr >> PAGE_SHIFT) * sizeof(uint64_t);
    if (lseek64(pagemap_fd, index, SEEK_SET) != (off64_t)index) {
        throw std::runtime_error("Can't seek to the correct page in virt2phys");
    }

    // Read the address (can return 0 of not in userspace)
    uint64_t pm_info;
    if (read(pagemap_fd, &pm_info, sizeof(uint64_t)) != sizeof(uint64_t))
        throw std::runtime_error("Can't find address in virt2phys, not in userspace?");

    // Check page is present and not swapped
    if ((pm_info & PAGE_PRESENT) && (!(pm_info & PAGE_SWAPPED))) {
        *paddr = ((pm_info & PAGE_PFN_MASK) << PAGE_SHIFT) + (vaddr & PAGE_MASK);
    } else {
        throw std::runtime_error("Page is not present or swapped in virt2phys");
    }
}

/**
 * @brief Check the contiquity of a memory allocation
 *
 * This will check if a specific virtual memory address is contigious in the physical
 * memory. It will also return the physical address of the virtual memory address.
 * @param[in] vaddr The virtual memory address
 * @param[in] size The size of the memory to check
 * @param[out] paddr The physical address of the virtual address
 * @return If the physical memory is contigious
 */
bool Bebop::checkContiguity(uint64_t vaddr, uint64_t size, uint64_t *paddr) {
    // First calculate the physical address
    virt2phys(vaddr, paddr);

    // Align to a page
    uint64_t curr_size = PAGE_SIZE - (*paddr & PAGE_MASK);
    uint64_t vcurrent = vaddr & ~PAGE_MASK;
    uint64_t pnext = (*paddr & ~PAGE_MASK) + PAGE_SIZE;

    // Check until checked whole size
    while(curr_size < size) {
        uint64_t pcurrent = 0;
        vcurrent += PAGE_SIZE;
        virt2phys(vcurrent, &pcurrent);

        if(pcurrent == pnext) {
            curr_size += PAGE_SIZE;
            pnext += PAGE_SIZE;
        } else {
            return false;
        }
    }

    return true;
}
