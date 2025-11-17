/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#pragma once

#include <iostream>
#include <functional>

 /**
  * @brief Base class for embedded GUI windows in the application.
  */
class EmbeddedWindow {
public:
    virtual ~EmbeddedWindow() = default;

    /**
     * @brief Renders the contents of the embedded window.
     */
    virtual void draw() = 0;

    /**
     * @brief Indicates whether the window is highlighted.
     * 
     * This can be used to signal special attention or status in the UI.
     * 
     * @return true if the window is highlighted, false otherwise
     */
    virtual bool highlighted() const {
        return false;
    }

    /**
     * @brief Saves the state of the window.
     * 
     * This method can be overridden by derived classes to implement
     * custom save functionality.
     */
    virtual void save() const {
        // Default implementation does nothing
    }
};

/**
 * @brief Template wrapper for non-EmbeddedWindow classes.
 * 
 * This template allows wrapping any class with a non-virtual draw method
 * to conform to the EmbeddedWindow interface.
 * 
 * @tparam T The class to be wrapped. Must implement a non-virtual draw method with no parameters.
 */
template <typename T>
class EmbeddedWindowWrapper : public EmbeddedWindow {
public:
    /**
     * @brief Constructor that takes an instance of the wrapped class.
     * 
     * @param window The instance of the class to be wrapped.
     */
    explicit EmbeddedWindowWrapper(T& window) : window_(window) {
        std::cout << "EmbeddedWindowWrapper created with window address: " << &window << std::endl;
    }

    /**
     * @brief Move constructor.
     */
    EmbeddedWindowWrapper(EmbeddedWindowWrapper&& other) noexcept = default;

    /**
     * @brief Move assignment operator.
     */
    EmbeddedWindowWrapper& operator=(EmbeddedWindowWrapper&& other) noexcept = default;

    /**
     * @brief Destructor.
     */
    virtual ~EmbeddedWindowWrapper() override = default;

    /**
     * @brief Renders the contents of the wrapped window.
     * 
     * This method calls the draw method of the wrapped class.
     */
    void draw() override {
        window_.draw();
    }

private:
    T& window_; ///< Reference to the wrapped window instance.
};

/**
 * @brief Wrapper for EmbeddedWindow using a lambda function.
 * 
 * This class allows wrapping a lambda function to conform to the EmbeddedWindow interface.
 */
class LambdaEmbeddedWindowWrapper : public EmbeddedWindow {
public:
    /**
     * @brief Constructor that takes a lambda function.
     * 
     * @param drawLambda The lambda function to be called in the draw method.
     */
    explicit LambdaEmbeddedWindowWrapper(std::function<void()> drawLambda) : drawLambda_(std::move(drawLambda)) {}

    /**
     * @brief Destructor.
     */
    virtual ~LambdaEmbeddedWindowWrapper() override = default;

    /**
     * @brief Renders the contents using the stored lambda function.
     */
    void draw() override {
        drawLambda_();
    }

private:
    std::function<void()> drawLambda_; ///< Lambda function to be called in the draw method.
};
