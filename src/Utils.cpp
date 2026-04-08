#include "Utils.h"

#include "Hooks.h"

namespace Utils {
    std::string FormIDToString(RE::FormID formID) {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return "0x0~Unknown";
        }

        uint8_t highByte = (formID >> 24) & 0xFF;

        if (highByte == 0xFE) {  // ESL
            uint16_t eslIndex = (formID >> 12) & 0xFFF;
            uint16_t localID = formID & 0xFFF;

            for (auto* file : dataHandler->files) {
                if (file && file->IsLight() && file->smallFileCompileIndex == eslIndex) {
                    return std::format("0x{:03X}~{}", localID, file->fileName);
                }
            }
        } else {  // regular plugin
            uint32_t localID = formID & 0xFFFFFF;

            for (auto* file : dataHandler->files) {
                if (file && file->compileIndex == highByte) {
                    return std::format("0x{:06X}~{}", localID, file->fileName);
                }
            }
        }
        return std::format("0x{:08X}~Unknown", formID);
    }

    RE::TESForm* ParseForm(const std::string& str) {
        auto pos = str.find('~');
        if (pos == std::string::npos) {
            return nullptr;
        }

        std::string idPart = str.substr(0, pos);
        std::string modName = str.substr(pos + 1);

        uint32_t localID = std::stoul(idPart, nullptr, 16);

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return nullptr;
        }

        const RE::TESFile* mod = dataHandler->LookupModByName(modName);
        if (!mod) {
            logger::error("Can't find mod: {}", modName);
            return nullptr;
        }

        // ESL (light plugin)
        if (mod->IsLight()) {
            localID &= 0xFFF;  // only lower 12 bits
        }

        RE::TESForm* form = dataHandler->LookupForm(localID, modName);
        if (!form) {
            logger::error("Can't find form: {:X}~{}", localID, modName);
            return nullptr;
        }

        return form;
    }

    void ApplySpeedToNode(RE::NiAVObject* node, float speed) {
        if (!node) return;

        // Check controller on this node
        if (auto* contMgr = node->GetController<RE::NiControllerManager>()) {
            for (auto* seq : contMgr->activeSequences) {
                if (seq) {
                    seq->frequency = speed;
                }
            }
        } else if (auto* contTime = node->GetController<RE::NiTimeController>()) {
            contTime->frequency = speed;
        }

        // If it's a NiNode recurse into children
        if (auto* niNode = node->AsNode()) {
            for (auto& child : niNode->GetChildren()) {
                ApplySpeedToNode(child.get(), speed);
            }
        }
    }

    void ApplyImpulseToNode(RE::NiAVObject* node, RE::hkVector4 velocity) {
        if (!node) return;
        if (auto* collisionObject = node->GetCollisionObject()) {
            if (auto* rigidBody = collisionObject->body->AsBhkRigidBody()) {
                rigidBody->SetLinearImpulse(velocity);
            }
        }

        // If it's a NiNode recurse into children
        if (auto* niNode = node->AsNode()) {
            for (auto& child : niNode->GetChildren()) {
                ApplyImpulseToNode(child.get(), velocity);
            }
        }
    }

    // This is a holy grail function that allows us to replace the model of any reference with any model.
    // This is done by directly loading the NIF file of the new model and applying it to the reference.
    void ReplaceModel(RE::TESObjectREFR* ref, std::string modelPath) {
        if (!ref) return;
        if (!modelPath.ends_with(".nif")) {
            logger::error("Model file is not a NIF: {}", modelPath);
            return;
        }
        SKSE::GetTaskInterface()->AddTask([ref, modelPath = std::move(modelPath)]() {
            logger::debug("Replacing model of {:08X} with {}", ref->GetFormID(), modelPath);
            RE::NiPointer<RE::NiNode> newModel{nullptr};
            if (RE::BSResource::ErrorCode::kNone == RE::BSModelDB::Demand(modelPath.c_str(), newModel, {})) {
                RE::NiPointer<RE::NiObject> deepCopy;
                if (newModel && newModel.get()) {
                    newModel->CreateDeepCopy(deepCopy);
                }
                RE::NiAVObject* constructedObject = deepCopy ? deepCopy->AsNode() : nullptr;
                if (constructedObject) {
                    ref->Set3D(constructedObject, false);
                }
            } else {
                logger::error("Failed to load model for replacement");
            }
        });
    }

    void ReplaceBaseObject(RE::TESObjectREFR* ref, RE::TESBoundObject* newBase) {
        if (!ref || !newBase) {
            logger::error("Invalid reference or base object for replacement");
            return;
        }
        ref->data.objectReference = newBase;
        ref->Disable();
        ref->Enable(false);
    }

    std::string GetModelPath(RE::TESBoundObject* base) {
        if (!base) return "";

        if (auto activator = base->As<RE::TESObjectACTI>()) {
            return activator->model.c_str();
        }
        if (auto alch = base->As<RE::AlchemyItem>()) {
            return alch->model.c_str();
        }
        if (auto book = base->As<RE::TESObjectBOOK>()) {
            return book->model.c_str();
        }
        if (auto ingr = base->As<RE::IngredientItem>()) {
            return ingr->model.c_str();
        }
        if (auto light = base->As<RE::TESObjectLIGH>()) {
            return light->model.c_str();
        }
        if (auto misc = base->As<RE::TESObjectMISC>()) {
            return misc->model.c_str();
        }
        if (auto stat = base->As<RE::TESObjectSTAT>()) {
            return stat->model.c_str();
        }
        if (auto movStat = base->As<RE::BGSMovableStatic>()) {
            return movStat->model.c_str();
        }
        if (auto tree = base->As<RE::TESObjectTREE>()) {
            return tree->model.c_str();
        }
        if (auto flora = base->As<RE::TESFlora>()) {
            return flora->model.c_str();
        }
        if (auto furn = base->As<RE::TESFurniture>()) {
            return furn->model.c_str();
        }
        if (auto weap = base->As<RE::TESObjectWEAP>()) {
            return weap->model.c_str();
        }
        if (auto ammo = base->As<RE::TESAmmo>()) {
            return ammo->model.c_str();
        }
        if (auto cont = base->As<RE::TESObjectCONT>()) {
            return cont->model.c_str();
        }
        if (auto door = base->As<RE::TESObjectDOOR>()) {
            return door->model.c_str();
        }
    }
}
        