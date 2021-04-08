#include "pch_RulrNodes.h"
#include "Base.h"

#include "../Database.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			namespace Channels {
				namespace Generator {
					//----------
					Base::Base() {
						RULR_NODE_INIT_LISTENER;
					}

					//----------
					string Base::getTypeName() const {
						return "Data::Channels::Base";
					}

					//----------
					void Base::init() {
						RULR_NODE_INSPECTOR_LISTENER;
						RULR_NODE_SERIALIZATION_LISTENERS;

						auto databaseInput = this->addInput<Database>();
						databaseInput->onNewConnection += [this](shared_ptr<Database> database) {
							this->connectDatabase(database);
						};
						databaseInput->onDeleteConnection += [this](shared_ptr<Database> database) {
							this->disconnectDatabase(database);
						};

						this->addressParameter.addListener(this, &Base::addressParameterCallback);
						this->addressParameter.set("Address", this->getTypeName());
					}

					//----------
					void Base::populateInspector(InspectArguments & inspectArgs) {
						auto inspector = inspectArgs.inspector;

						inspector->addEditableValue(this->addressParameter);
					}

					//----------
					void Base::serialize(nlohmann::json & json) {
						Utils::serialize(json, this->addressParameter);
					}

					//----------
					void Base::deserialize(const nlohmann::json & json) {
						Utils::deserialize(json, this->addressParameter);
					}

					//----------
					const Address & Base::getAddress() const {
						return this->address;
					}

					//----------
					void Base::connectDatabase(shared_ptr<Database> database) {
						database->addGenerator(static_pointer_cast<Base>(this->shared_from_this()));
					}

					//----------
					void Base::disconnectDatabase(shared_ptr<Database> database) {
						if (database) {
							database->removeGenerator(this);
						}
					}

					//----------
					void Base::addressParameterCallback(string &) {
						this->address = Address(this->addressParameter.get());
					}
				}
			}
		}
	}
}