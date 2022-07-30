#pragma once
class MathExtensionClass
{
private:
    void swap(int &a, int &b);
    int partition(int arr[], int start, int end);

public:
    void quickSort(int arr[], int start, int end);
};
extern class MathExtensionClass MathExtension;
