Matrix3d m = Matrix3d::random();
cout << "Here is the matrix m:" << endl << m << endl;
Matrix3d inv;
m.computeInverse(&inv);
cout << "Its inverse is:" << endl << inv << endl;
