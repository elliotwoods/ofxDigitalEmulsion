#pragma once

#include <memory>
#include <vector>
#include "ofLog.h"

namespace ofxRulr {
	namespace Utils {
		template<typename BaseType>
		class Set : public std::vector<std::shared_ptr<BaseType> > {
		public:
			template<typename T>
			std::shared_ptr<T> get()  const {
				for(auto item : * this) {
					auto castItem = dynamic_pointer_cast<T>(item);
					if (castItem != NULL) {
						return castItem;
					}
				}
				ofLogError("ofxRulr") << "Item of type [" << T().getTypeName() << "] could not be found in Set<" << typeid(BaseType).name() << ">";
				return shared_ptr<T>();
			}

			void update() {
				for(auto item : * this) {
					item->update();
				}
			}

			void add(std::shared_ptr<BaseType> item) {
				this->push_back(item);
			}

			void remove(std::shared_ptr<BaseType> item) {
				auto it = find(this->begin(), this->end(), item);
				if (it != this->end()) {
					this->erase(it);
				}
				else {
					ofLogError("ofxRulr") << "Pin of [" << item << "] could not be found in Set<" << typeid(BaseType).name() << "> and therefore could not be removed.";
				}
			}
		};
	}
}