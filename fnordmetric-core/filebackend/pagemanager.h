/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * Licensed under the MIT license (see LICENSE).
 */
#ifndef _FNORDMETRIC_FILEBACKEND_PAGEMANAGER_H
#define _FNORDMETRIC_FILEBACKEND_PAGEMANAGER_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <memory>
#include "../storagebackend.h"

namespace fnordmetric {
namespace filebackend {

/**
 * This is an internal class. For usage instructions and extended documentation
 * please refer to "storagebackend.h" and "filebackend.h"
 */
class PageManager {
  friend class FileBackendTest;
public:
  struct Page {
    uint64_t offset;
    uint64_t size;
  };

  PageManager(size_t end_pos_, size_t block_size);
  PageManager(const PageManager& copy) = delete;
  PageManager& operator=(const PageManager& copy) = delete;
  PageManager(const PageManager&& move);

  /**
   * Request a new page from the page manager
   */
  Page allocPage(size_t min_size);

  /**
   * Return a page to the pagemanager. Adds this page to the freelist
   */
  void freePage(const Page& page);

protected:

  /**
   * Try to find a free page with a size larger than or equal to min_size
   *
   * Returns true if a matching free page was found and returns the page into
   * the destination parameter. Returns false if no matching page was found and
   * does not change the destination parameter.
   */
  bool findFreePage(size_t min_size, Page* destination);

  /**
   * Index of the first unused byte in the file
   */
  size_t end_pos_;

  /**
   * Optimal block size for the underlying file
   */
  const size_t block_size_;

  /**
   * Page free list
   *
   * tuple is (size, offset)
   */
  std::vector<std::pair<uint64_t, uint64_t>> freelist_;
};

class MmapPageManager {
  friend class FileBackendTest;
protected:
  struct MmappedFile {
    void* data;
    const size_t size;
    const int fd;
    size_t refs;
    MmappedFile(void* __data, const size_t __size, const int __fd);
    void incrRefs();
    void decrRefs();
  };

public:
  /**
   * Size of the initially create mmaping in bytes. All mmapings will be a
   * multiple of this size!
   */
  static const size_t kMmapSizeMultiplier = 1048576; /* 1 MB */

  struct MmappedPageRef {
    const PageManager::Page page;
    MmappedFile* file;
    MmappedPageRef(const PageManager::Page& __page, MmappedFile* __file);
    ~MmappedPageRef();
    MmappedPageRef(const MmappedPageRef& copy) = delete;
    MmappedPageRef& operator=(const MmappedPageRef& copy) = delete;
    void* operator->() const;
    void* operator*() const;
    MmappedPageRef(const MmappedPageRef&& move);
  };

  /**
   * Create a new mmap page manager and hand over ownership of the provided
   * filedescriptor.
   */
  static MmapPageManager* openFile(int fd);

  MmapPageManager(const MmapPageManager& copy) = delete;
  MmapPageManager& operator=(const MmapPageManager& copy) = delete;
  ~MmapPageManager();

  /**
   * Request a new page to be mapped into memory. Returns a smart pointer.
   */
  MmappedPageRef allocPage(size_t min_size);

  /**
   * Request an exisiting page to be mapped into memory. Returns a smart pointer.
   */
  MmappedPageRef getPage(uint64_t offset, size_t size);

protected:
  explicit MmapPageManager(int fd, PageManager&& page_manager);

  /**
   * Returns a mmap()ed memory region backend by the managed file spans until
   * at least last_byte
   */
  MmappedFile* getMmapedFile(uint64_t last_byte);

  const int fd_;
  PageManager page_manager_;
  MmappedFile* current_mapping_;
};

}
}
#endif
