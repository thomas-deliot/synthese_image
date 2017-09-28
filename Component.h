#pragma once

class GameObject;

/*!
*  \brief Classe de base dont hérite tous les composants.
*/
class Component
{
protected:
	/*!
	*  \brief le GameObject auquel ce composant est assigné.
	*/
	GameObject* gameObject = nullptr;

public:
	/*!
	*  \brief fonction appelée au lancement de l'application pour tous les composants.
	*/
	virtual void OnStart() {}

	/*!
	*  \brief fonction appelée à la fermeture de l'application pour tous les composants.
	*/
	virtual void OnDestroy() {}

	/*!
	*  \brief fonction appelée à chaque frame de l'application pour tous les composants.
	*  \param dt le temps en millisecondes écoulé depuis la dernière frame.
	*/
	virtual void Update(float dt) {}

	/*!
	*  \brief Assigne le GameObject auquel ce composant appartient.
	*  \param gameobject le GameObject auquel ce composant appartient.
	*/
	void SetGameObject(GameObject* gameobject)
	{
		this->gameObject = gameobject;
	}

	/*!
	*  \brief Récupère le GameObject auquel ce composant appartient.
	*  \return le GameObject auquel ce composant appartient.
	*/
	GameObject* GetGameObject()
	{
		return this->gameObject;
	}
};
