#include <Python.h>
#include <iostream>

int main() {
    // Initialize the Python interpreter
    Py_Initialize();

    if (!Py_IsInitialized()) {
        std::cerr << "Failed to initialize the Python interpreter." << std::endl;
        return 1;
    }

    // Execute a Python script
    FILE* file = fopen("example.py", "r");
    if (file == nullptr) {
        std::cerr << "Failed to open the Python script file." << std::endl;
        Py_Finalize();
        return 1;
    }

    // Run the Python script
    int result = PyRun_SimpleFile(file, "example.py");
    if (result != 0) {
        std::cerr << "Failed to execute the Python script." << std::endl;
    }

    // Close the file handler
    fclose(file);

    // Finalize the Python interpreter
    Py_Finalize();

    return 0;
}
