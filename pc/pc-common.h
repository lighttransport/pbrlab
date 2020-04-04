#ifndef PBRLAB_PC_COMMON_H_
#define PBRLAB_PC_COMMON_H_
#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "material-param.h"
#include "render-layer.h"
#include "scene.h"

class EditQueue {
public:
  explicit EditQueue(void);
  virtual ~EditQueue(void);

  enum FileType { kVoid = 0, kFloat, kPbrlabMaterialParameter };

  template <typename T>
  inline void Push(const T* src, T* dst) {
    void* dst_ = reinterpret_cast<void*>(dst);

    FileType file_type = kVoid;
    if (std::is_same<float, T>::value) {
      file_type = kFloat;
    } else if (std::is_same<pbrlab::MaterialParameter, T>::value) {
      file_type = kPbrlabMaterialParameter;
    } else {
      // TODO error log
      assert(false);
      return;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.count(dst_) > 0 && queue_[dst_].first != nullptr) {
      delete reinterpret_cast<T*>(queue_[dst_].first);
    }

    queue_[dst_].first  = reinterpret_cast<void*>(new T(*src));
    queue_[dst_].second = file_type;
  }

  template <typename T>
  inline bool FetchIfExit(const T* ptr, T* dst) {
    std::lock_guard<std::mutex> lock(mtx_);
    void* ptr_ = const_cast<void*>(reinterpret_cast<const void*>(ptr));
    if (queue_.count(ptr_) == 0) {
      return false;
    }

    // Type check
    if (std::is_same<float, T>::value) {
      if (queue_[ptr_].second != kFloat) {
        return false;
      }
    } else if (std::is_same<pbrlab::MaterialParameter, T>::value) {
      if (queue_[ptr_].second != kPbrlabMaterialParameter) {
        return false;
      }
    } else {
      // TODO error log
      assert(false);

      return false;
    }

    const T* src = reinterpret_cast<T*>(queue_[ptr_].first);

    (*dst) = (*src);

    return true;
  }

  void EditAndPopAll(void);

protected:
  std::unordered_map<void* /*dst*/, std::pair<void* /*src*/, FileType>> queue_;
  mutable std::mutex mtx_;
};

struct RenderItem {
  RenderItem(void);

  pbrlab::Scene scene;
  pbrlab::RenderLayer layer;

  std::atomic_bool cancel_render_flag;
  std::atomic_bool finish_frag;

  std::atomic_size_t last_render_pass;
  std::atomic_size_t finish_pass;
  std::atomic_size_t max_pass;

  EditQueue edit_queue;
};

bool CreateScene(int argc, char** argv, pbrlab::Scene* scene);

#endif  // PBRLAB_PC_COMMON_H_
