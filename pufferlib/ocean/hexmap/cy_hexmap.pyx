cimport numpy as cnp
from libc.stdlib cimport calloc, free

cdef extern from "hexmap.h":

    ctypedef struct HexCoord:
        int q
        int r
        int index

    ctypedef struct Hexmap:
        unsigned char* observations
        int* actions
        float* rewards
        unsigned char* terminals
        int radius
        int size
        int tick
        int q
        int r
        int thirst
        int hunger
        HexCoord target_coord
        HexCoord* terrain_coordinates
        unsigned char* terrain_elements

    ctypedef struct Client

    void reset(Hexmap* env)
    void step(Hexmap* env)
    Client* make_client(Hexmap* env)
    void close_client(Client* client)
    void render(Client* client, Hexmap* env)

cdef class CyHexmap:
    cdef:
        Hexmap* envs
        Client* client
        int num_envs
        int size

    def __init__(self, unsigned char[:, :] observations, int[:] actions,
            float[:] rewards, unsigned char[:] terminals, int num_envs, int size):

        self.envs = <Hexmap*> calloc(num_envs, sizeof(Hexmap))
        self.num_envs = num_envs
        self.client = NULL

        cdef int i
        for i in range(num_envs):
            self.envs[i] = Hexmap(
                observations = &observations[i, 0],
                actions = &actions[i],
                rewards = &rewards[i],
                terminals = &terminals[i],
                size=size,
            )

    def reset(self):
        cdef int i
        for i in range(self.num_envs):
            reset(&self.envs[i])

    def step(self):
        cdef int i
        for i in range(self.num_envs):
            step(&self.envs[i])

    def render(self):
        cdef Hexmap* env = &self.envs[0]
        if self.client == NULL:
            self.client = make_client(env)

        render(self.client, env)

    def close(self):
        if self.client != NULL:
            close_client(self.client)
            self.client = NULL

        free(self.envs)
