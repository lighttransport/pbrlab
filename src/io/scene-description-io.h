#ifndef PBRLAB_SCENE_DESCRIPTION_IO_H_
#define PBRLAB_SCENE_DESCRIPTION_IO_H_

#include <string>

#include "scene-description/scene-description.h"

namespace pbrlab {
namespace io {

bool LoadSceneDescriptionFromJson(
    const std::string& filepath,
    scene_description::Root* scene_description_root);

}
}  // namespace pbrlab

#endif  // PBRLAB_SCENE_DESCRIPTION_IO_H_
