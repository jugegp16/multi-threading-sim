# multi-threading-sim
Created a multi-threading program to handle two procedures, visitors and tour guides for a museum simulation. I utilized monitors -- mutexs and condition variables -- to synchronize the processes.

Problem details
-----------------
Imagine that a museum has been established to showcase old operating systems from all over the history of computing. Imagine also that a number of students have volunteered to work as tour guides to answer the questions of the museum visitors and help them run the showcased operating systems.

The museum is hosted in a room that can hold up to 20 visitors and 2 tour guides at a time (with COVID-19 social distancing restrictions in place). In order not to overwhelm the volunteer tour guides, each volunteer guide is limited to supervise at most 10 visitors each day, and is free to leave afterwards. However, if there is more than one guide inside the museum, all guides will wait for each other before leaving, and leave together.

To provide good quality of service to the visitors, it should never be the case that the number of visitors inside the museum exceeds 10 times the number of tour guides who are inside the museum.

The museum has an associated ticket booth. Each morning, a number of tickets is made available based on the number of visitors which will arrive and the number of tour guides that volunteered to show up on that day. Visitors claim the tickets in no particular order. Once there are no more tickets, any arriving visitors are turned away. Once no more visitors will arrive, any remaining guides can leave.
