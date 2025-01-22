#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <assert.h>
#include <utility>



// Global Variables
const unsigned int GRID_ROWS = 5;
const unsigned int GRID_COLUMNS = 10;
const sf::Font FONT("calibri.ttf");



// Functions
template<class T>
std::string convertToString(T variable)
{
	std::stringstream s;
	s << variable;
	return s.str();
}
static sf::RenderWindow* makeWindow()
{
	std::string winTitle = "SFML Project";
	sf::Vector2u winSize{ 800u, 600u };
	unsigned int winFPS = 0u;
	bool winState = false;
	bool winVSync = true;
	bool winRepeatedKey = true;
	
	sf::ContextSettings settings;
	settings.antiAliasingLevel = 8; // Default 8

	std::ifstream ifs("Window.ini");
	if (ifs.is_open())
	{
		std::getline(ifs, winTitle);
		ifs >> winSize.x >> winSize.y;
		ifs >> winFPS;
		ifs >> winState;
		ifs >> winVSync;
		ifs >> winRepeatedKey;
		ifs >> settings.antiAliasingLevel;

		ifs.close();
	}

	sf::Image windowIcon;
	if (!windowIcon.loadFromFile("icon.png"))
	{
		// Handle Loading Issue
		std::cout << "Couldn't Load Game Icon!" << '\n';
	}

	sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode(winSize), winTitle, static_cast<sf::State>(winState), settings);
	window->setIcon({ windowIcon.getSize().x, windowIcon.getSize().y }, windowIcon.getPixelsPtr());
	window->setFramerateLimit(winFPS);
	window->setVerticalSyncEnabled(winVSync);
	window->setKeyRepeatEnabled(winRepeatedKey);

	return window;
}



// Classes
class Grid
{
public:

	Grid() : m_grid()
	{

		sf::Vector2f previousPosition({ 0, 120.f });

		for (unsigned int row = 0; row < m_grid.size(); ++row)
		{
			for (unsigned int column = 0; column < m_grid[row].size(); ++column)
			{
				m_grid[row][column].first = new sf::RectangleShape({ 60.f, 60.f });
				m_grid[row][column].first->setFillColor(sf::Color::Transparent);
				m_grid[row][column].first->setOutlineThickness(1.f);
				m_grid[row][column].first->setOutlineColor(sf::Color::Green);
				m_grid[row][column].first->setPosition({ previousPosition.x + 120.f, previousPosition.y });
				m_grid[row][column].second = false; // false = Free

				previousPosition = m_grid[row][column].first->getPosition();
			}
			previousPosition = { 0 , previousPosition.y + 120.f };
		}
	}
	~Grid()
	{
		for (unsigned int row = 0; row < m_grid.size(); ++row)
			for (unsigned int column = 0; column < m_grid[row].size(); ++column)
			{
				delete m_grid[row][column].first;
			}
	}

	sf::Vector2f getPosition(unsigned short int slot) const
	{
		assert(slot >= 1 && slot <= size());
		return m_grid[(slot - 1) / GRID_COLUMNS][(slot - 1) % GRID_COLUMNS].first->getPosition();
	}
	bool isOccupied(unsigned short int slot) const
	{
		assert(slot >= 1 && slot <= size());
		return m_grid[(slot - 1) / GRID_COLUMNS][(slot - 1) % GRID_COLUMNS].second;
	}
	void setStatus(unsigned short int slot, bool status)
	{
		assert(slot >= 1 && slot <= size());
		m_grid[(slot - 1) / GRID_COLUMNS][(slot - 1) % GRID_COLUMNS].second = status;
	}

	unsigned int size() const
	{
		return GRID_ROWS * GRID_COLUMNS;
	}

	void draw(sf::RenderWindow* window)
	{
		for (unsigned int row = 0; row < m_grid.size(); ++row)
		for (unsigned int column = 0; column < m_grid[row].size(); ++column)
		{
			if (!m_grid[row][column].second)
				window->draw(*m_grid[row][column].first);
		}
	}

private:

	std::array<std::array<std::pair<sf::RectangleShape*, bool>, /*columns*/ GRID_COLUMNS>, /*rows*/ GRID_ROWS> m_grid;
};

template<class T>
class Node
{
public:

	Node(Grid* grid, T data = T(), Node<T>* node = nullptr) : m_grid(grid), m_prev(node), m_data(data), m_slot(0)
	{
		initShape();
		initText();
	}
	~Node()
	{
		m_grid->setStatus(m_slot, false);

		m_grid = nullptr;
		delete m_text;
		delete m_shape;
	}

	void draw(sf::RenderWindow* window) const
	{
		window->draw(*m_shape);
		window->draw(*m_text);
	}

	T getData() const
	{
		return m_data;
	}

	Node<T>* getPrev() const
	{
		return m_prev;
	}
	void setPrev(Node<T>* node)
	{
		m_prev = node;
	}

private:

	void initText()
	{
		sf::String content = convertToString(m_data);

		m_text = new sf::Text(FONT, content);
		m_text->setFillColor(sf::Color::Red);
		m_text->setPosition(m_shape->getPosition());
	}

	void initShape()
	{
		m_shape = new sf::CircleShape(50.0f);
		m_shape->setOrigin({ 25.0f, 25.0f });

		for (unsigned int slot = 1; slot <= m_grid->size(); ++slot)
			if (!m_grid->isOccupied(slot))
			{
				m_shape->setPosition(m_grid->getPosition(slot));
				m_grid->setStatus(slot, true);
				m_slot = slot;
				break;
			}

		m_shape->setFillColor(sf::Color::White);
	}

	T				 m_data;
	sf::CircleShape* m_shape;
	sf::Text*		 m_text;
	Node<T>*		 m_prev;

	Grid*			 m_grid;
	unsigned int	 m_slot;

};



class DataStructure
{
public:

	DataStructure(Grid* grid, unsigned int size = 0) : m_grid(grid), m_count(size) {}

	virtual bool empty() const
	{
		return !m_count;
	}

	virtual unsigned int size() const
	{
		return m_count;
	}

	virtual void draw(sf::RenderWindow* window) const = 0;

protected:

	unsigned int m_count;
	Grid*	 m_grid;
};

template<class T>
class Stack : public DataStructure
{
public:

	Stack(Grid* grid, unsigned int size = 0, T data = T()) : DataStructure(grid, 0), m_top(nullptr) {
		while (size--)
		{
			push(data);
		}
	}

	~Stack()
	{
		clear();
	}

	void push(T data)
	{
		Node<T>* newNode = new Node<T>(m_grid, data, nullptr);

		if (!empty())
			newNode->setPrev(m_top);
		
		m_top = newNode;
		++m_count;

		newNode = nullptr;
	}

	void pop()
	{
		if (empty()) return;

		Node<T>* temp = m_top;
		m_top = m_top->getPrev();

		--m_count;

		delete temp;
		temp = nullptr;
	}

	void clear()
	{
		while (m_count)
		{
			pop();
		}
	}

	std::string print() const
	{
		std::string output = "";

		if (empty())
		{
			output = "[ ]";
			std::cout << output << '\n';
			return output;
		}

		Node<T>* temp = m_top;

		output = "[ ";
		while (temp)
		{
			output += (convertToString(temp->getData()) + ", ");
			temp = temp->getPrev();
		}
		output += "\b\b ]";

		temp = nullptr;

		std::cout << output << '\n';
		return output;
	}

	void draw(sf::RenderWindow* window) const override
	{
		Node<T>* temp = m_top;
		while (temp)
		{
			temp->draw(window);
			temp = temp->getPrev();
		}

		temp = nullptr;
	}

	T top() const
	{
		return m_top->getData();
	}

private:
	
	Node<T>* m_top;
};

template<class T>
class Queue : public DataStructure
{
public:

	Queue(Grid* grid, unsigned int size = 0, T data = T())
		: DataStructure(grid, 0), m_head(nullptr), m_tail(nullptr)
	{
		while (size--)
		{
			Enqueue(data);
		}
	}

	~Queue()
	{
		clear();
	}

	void Enqueue(T data)
	{
		Node<T>* newNode = new Node<T>(m_grid, data, nullptr);

		if (!empty())
			m_tail->setPrev(newNode);
		else
			m_head = newNode;
		m_tail = newNode;

		++m_count;

		newNode = nullptr;
	}

	void Dequeue()
	{
		if (empty()) return;

		Node<T>* temp = m_head;
		m_head = m_head->getPrev();

		--m_count;

		delete temp;
		temp = nullptr;
	}

	void clear()
	{
		while (!empty())
		{
			Dequeue();
		}
	}

	std::string print() const
	{
		std::string output = "";

		if (empty())
		{
			output = "[ ]";
			std::cout << output << '\n';
			return output;
		}

		Node<T>* temp = m_head;

		output = "[ ";
		while (temp)
		{
			output += (convertToString(temp->getData()) + ", ");
			temp = temp->getPrev();
		}
		output += "\b\b ]";

		temp = nullptr;

		std::cout << output << '\n';
		return output;
	}

	void draw(sf::RenderWindow* window) const override
	{
		Node<T>* temp = m_head;
		while (temp)
		{
			temp->draw(window);
			temp = temp->getPrev();
		}

		temp = nullptr;
	}

	T head() const
	{
		return m_head->getData();
	}
	
	T tail() const
	{
		return m_tail->getData();
	}

private:
	
	Node<T>* m_head;
	Node<T>* m_tail;
};



class Button
{
public:

	Button(sf::String text = "Placement Text!") : m_shape(nullptr), m_text(nullptr)
	{
		initShape();
		initText(text);
	}
	virtual ~Button()
	{
		delete m_text;
		delete m_shape;
	}

	virtual void draw(sf::RenderWindow* window)
	{
		window->draw(*m_shape);
		window->draw(*m_text);
	}

private:

	void initText(sf::String text)
	{
		sf::String content = text;

		m_text = new sf::Text(FONT, content);
		m_text->setFillColor(sf::Color::Black);
		m_text->setPosition(m_shape->getPosition());
	}
	void initShape()
	{
		m_shape = new sf::RectangleShape({ 200.f, 100.f });
		m_shape->setOrigin({ 100.0f, 50.0f });
		m_shape->setPosition({ 720.f, 20.f });
		m_shape->setFillColor(sf::Color::Green);
	}

	sf::RectangleShape* m_shape;
	sf::Text* m_text;
};

template<class T>
class PushButton : public Button
{
public:

	PushButton() : Button("Push") {}

	void action(Queue<T>* dataStructure)
	{
		dataStructure->Enqueue(T());
	}

};



int main()
{

	Grid* grid = new Grid();

	Queue<std::string> s(grid);
	
	PushButton<int> pushButton;
	

	sf::RenderWindow* window = makeWindow();

	// run the program as long as the window is open
	while (window->isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		while (const std::optional event = window->pollEvent())
		{
			// "close requested" event: we close the window
			if (event->is<sf::Event::Closed>())
				window->close();

			if (const auto* e = event->getIf<sf::Event::MouseButtonPressed>())
			{
				//if (e->position.x ==)
				std::cout << "Mouse Clicked!\n";
				switch (e->button)
				{
				case sf::Mouse::Button::Left:
					s.Enqueue("hello");
					break;
				case sf::Mouse::Button::Right:
					s.Dequeue();
					break;
				}
			}
		}

		// clear the window with black color
		window->clear(sf::Color::Black);

		grid->draw(window);
		s.draw(window);
		pushButton.draw(window);

		// end the current frame
		window->display();
	}

	return 0;
}