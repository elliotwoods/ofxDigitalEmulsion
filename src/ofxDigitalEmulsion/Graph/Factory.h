#pragma once

#include "Node.h"
#include "../Utils/Initialiser.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		class BaseFactory {
		public:
			virtual string getNodeTypeName() = 0;
			virtual shared_ptr<Node> make() = 0;
		};

		//----------
		template<typename NodeType>
		class Factory : public BaseFactory {
		public:
			string getNodeTypeName() override {
				return NodeType().getTypeName();
			}

			shared_ptr<Node> make() override {
				return static_pointer_cast<Node>(this->makeTyped());
			}

			shared_ptr<NodeType> makeTyped() {
				auto node = make_shared<NodeType>();
				node->init();
				return node;
			}
		};

		//----------
		class FactoryRegister : public map<string, shared_ptr<BaseFactory>> {
		public:
			static FactoryRegister & X();

			void add(shared_ptr<BaseFactory>);

			template<typename NodeType>
			shared_ptr<Factory<NodeType>> get() {
				auto untypedFactory = this->get(NodeType().getTypeName());
				auto typedFactory = dynamic_cast<Factory<NodeType>>(untypedFactory);
				return typedFactory;
			}

			shared_ptr<BaseFactory> get(string nodeTypeName);
		protected:
			static FactoryRegister * singleton;
		};
	}
}