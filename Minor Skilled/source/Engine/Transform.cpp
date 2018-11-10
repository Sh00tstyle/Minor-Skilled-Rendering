#include "Transform.h"

Transform::Transform(glm::vec3 localPosition): localTransform(glm::mat4(1.0f)), worldTransform(glm::mat4(1.0f)) {
	translate(localPosition);
}

Transform::~Transform() {
}

void Transform::decompose() {
	//matrix decomposition (dark magic): https://stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
	glm::decompose(localTransform, _localScale, _localRotation, _localPosition, _localSkew, _localPerspective);
	glm::decompose(worldTransform, _worldScale, _worldRotation, _worldPosition, _worldSkew, _worldPerspective);

	_localRotation = glm::conjugate(_localRotation);
	_worldRotation = glm::conjugate(_worldRotation);
}

void Transform::translate(glm::vec3 translation) {
	localTransform = glm::translate(localTransform, translation);
}

void Transform::rotate(float angle, glm::vec3 axis) {
	localTransform = glm::rotate(localTransform, glm::radians(angle), axis);
}

void Transform::scale(glm::vec3 scale) {
	localTransform = glm::scale(localTransform, scale);
}

glm::vec3 Transform::getLocalScale() {
	return _localScale;
}

glm::quat Transform::getLocalRotation() {
	return _localRotation;
}

glm::vec3 Transform::getLocalPosition(bool decomposed) {
	if(decomposed) return _localPosition;
	else return glm::vec3(localTransform[3]);
}

glm::vec3 Transform::getLocalSkew() {
	return _localSkew;
}

glm::vec3 Transform::getLocalPerspective() {
	return _localPerspective;
}

glm::vec3 Transform::getWorldScale() {
	return _worldScale;
}

glm::quat Transform::getWorldRotation() {
	return _worldRotation;
}

glm::vec3 Transform::getWorldPosition(bool decomposed) {
	if(decomposed) return _worldPosition;
	else return glm::vec3(worldTransform[3]);
}

glm::vec3 Transform::getWorldSkew() {
	return _worldSkew;
}

glm::vec3 Transform::getWorldPerspective() {
	return _worldPerspective;
}