"""Smoke test for the ogre-oggsound wheel.

Requires ogre-python to be installed in the same environment.
"""
import Ogre
import Ogre.OggSound as OggSound


def main():
    ogre_root = Ogre.Root("", "", "")
    sound_root = OggSound.Root()
    sound_root.initialise()
    assert OggSound.SoundManager.getSingleton() is not None
    sound_root.shutdown()
    ogre_root.shutdown()
    print("ogre-oggsound OK")


if __name__ == "__main__":
    main()
