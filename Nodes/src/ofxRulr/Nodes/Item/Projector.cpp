#include "pch_RulrNodes.h"
#include "Projector.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Projector::Projector() : View(false) {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Projector::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
			}

			//----------
			string Projector::getTypeName() const {
				return "Item::Projector";
			}

			//----------
			void Projector::serialize(nlohmann::json & json) {

			}

			//----------
			void Projector::deserialize(const nlohmann::json & json) {

			}

			//----------
			void Projector::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::EditableValue<float>("Resolution width", [this]() {
					return this->getWidth();
				}, [this](string valueString) {
					const auto value = ofToFloat(valueString);
					if (value > 0) {
						this->setWidth(value);
					}
				}));
				inspector->add(new Widgets::EditableValue<float>("Resolution height", [this]() {
					return this->getHeight();
				}, [this](string valueString) {
					const auto value = ofToFloat(valueString);
					if (value > 0) {
						this->setHeight(value);
					}
				}));
			}
		}
	}
}