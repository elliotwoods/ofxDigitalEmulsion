#pragma once

#include "ofxCvGui/Element.h"
#include "ofxRulr/Utils/Constants.h"
#include "ofxRulr/Utils/LambdaDrawable.h"

namespace ofxRulr {
	namespace Nodes {
		class Base;
	}

	namespace Graph {
		namespace Editor {
			/// A class to draw a preview icon for a Node type
			class OFXRULR_API_ENTRY PinView : public ofxCvGui::Element {
			public:
				PinView();

				void setup(Nodes::Base & node);

				template<typename NodeType>
				void setup() {
					NodeType tempNode;
					this->setup(tempNode);
				}
			protected:
				shared_ptr<Utils::LambdaDrawable> icon;
				string nodeTypeName;
			};
		}
	}
}