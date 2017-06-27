earthEllipsoid = {6378137.0, 6378137.0, 6356752.314245} -- Earth's radii
return {
    -- Earth barycenter module
    {
        Name = "EarthBarycenter",
        Parent = "SolarSystemBarycenter",
        Transform = {
            Translation = {
                Type = "SpiceTranslation",
                Body = "EARTH",
                Observer = "SUN",
                Kernels = "${OPENSPACE_DATA}/spice/de430_1850-2150.bsp"
            },
        },
    },
    {
    -- The default reference frame for Earth-orbiting satellites
        Name = "EarthInertial",
        Parent = "EarthBarycenter",
        Transform = {
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "J2000",
                DestinationFrame = "GALACTIC",
            }
        },
    },
    -- EarthTrail module
    {
        Name = "EarthTrail",
        Parent = "SolarSystemBarycenter",
        Renderable = {
            Type = "RenderableTrailOrbit",
            Translation = {
                Type = "SpiceTranslation",
                Body = "EARTH",
                Observer = "SUN"
            },
            Color = { 0.5, 0.8, 1.0 },
            -- StartTime = "2016 JUN 01 12:00:00.000",
            -- EndTime = "2017 JAN 01 12:00:00.000",
            -- SampleInterval = 3600
            Period = 365.242,
            Resolution = 1000
        },
        GuiName = "/Solar/EarthTrail"
    },
    {
        Name = "SolarImagery_EarthMarker_Marker",
        Parent = "Earth",
        Renderable = {
            Type = "RenderablePlane",
            Size = 10^9.8,
            Origin = "Center",
            Billboard = true,
            Texture = solarImageryDataRootPath .. "/markers/marker_earth.png",
            BlendMode = "Additive"
        },
        Transform = {
            Translation = {
                Type = "StaticTranslation",
                Position = {0, 0, 0},
            }
        }
    },
    -- RenderableGlobe module
    {
        Name = "Earth",
        Parent = "EarthBarycenter",
        Transform = {
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "IAU_EARTH",
                DestinationFrame = "GALACTIC",
            },
            Scale = {
                Type = "StaticScale",
                Scale = 1,
            },
        },
        Renderable = {
            Type = "RenderableGlobe",
            Radii = earthEllipsoid,
            CameraMinHeight = 300,
            InteractionDepthBelowEllipsoid = 0, -- Useful when having negative height map values
            SegmentsPerPatch = 64,
            Layers = {
                ColorLayers = {
                    {
                        Name = "ESRI VIIRS Combo",
                        Type = "ByLevel",
                        LevelTileProviders = {
                            {
                                MaxLevel = 3,
                                TileProvider = {
                                    Type = "Temporal",
                                    Name = "Temporal VIIRS SNPP",
                                    FilePath = "map_service_configs/GIBS/Temporal_VIIRS_SNPP_CorrectedReflectance_TrueColor.xml", },
                            },
                            {
                                MaxLevel = 22,
                                TileProvider = {
                                    Name = "ESRI Imagery World 2D",
                                    FilePath = "map_service_configs/ESRI/ESRI_Imagery_World_2D.wms"
                                },
                            },
                        },
                        Enabled = true,
                    },
                    {
                        Name = "Terra",
                        FilePath = "map_service_configs/GIBS/MODIS_Terra_CorrectedReflectance_TrueColor.xml"
                    },
                    {
                        Name = "ESRI",
                        FilePath = "map_service_configs/ESRI/ESRI_Imagery_World_2D.wms"
                    },
                    {
                        Type = "Temporal",
                        Name = "Temporal_GHRSST_L4_MUR_Sea_Surface_Temperature",
                        FilePath = "map_service_configs/GIBS/Temporal_GHRSST_L4_MUR_Sea_Surface_Temperature.xml",
                    },
                    {
                        Type = "Temporal",
                        Name = "Temporal_AMSR2_GCOM_W1_Sea_Ice_Concentration",
                        FilePath = "map_service_configs/GIBS/Temporal_AMSR2_GCOM_W1_Sea_Ice_Concentration.xml",
                    },
                    {
                        Name = "BMNG",
                        FilePath = "map_service_configs/Utah/Bmng.wms"
                    },
                    {
                        Name = "Ovation Prime",
                        Type = "Temporal",
                        FilePath = "map_service_configs/other/ovation_12_00.xml",
                        TilePixelSize = 64,
                        Enabled = true
                    }
                },
                GrayScaleLayers = { },
                GrayScaleColorOverlays = { },
                NightLayers = {
                    {
                        Name = "Earth at Night 2012",
                        FilePath = "map_service_configs/GIBS/VIIRS_CityLights_2012.xml",
                        Enabled = true,
                        Settings = {
                            Opacity = 1.0,
                            Gamma = 1.5,
                            Multiplier = 15.0,
                        },
                    },
                    {
                        Type = "Temporal",
                        Name = "Temporal Earth at Night",
                        FilePath = "map_service_configs/GIBS/Temporal_VIIRS_SNPP_DayNightBand_ENCC.xml"
                    },
                    {
                        Name = "Ovation Prime",
                        Type = "Temporal",
                        FilePath = "map_service_configs/other/ovation_1m.xml",
                        TilePixelSize = 64,
                        Enabled = true
                    }
                },
                WaterMasks = {
                    {
                        Name = "MODIS_Water_Mask",
                        FilePath = "map_service_configs/GIBS/MODIS_Water_Mask.xml",
                        Enabled = true,
                    },
                    {
                        Name = "GEBCO",
                        FilePath = "map_service_configs/Utah/Gebco.wms",
                    }
                },
                ColorOverlays = {
                    {
                        Name = "Coastlines",
                        FilePath = "map_service_configs/GIBS/Coastlines.xml",
                    },
                    {
                        Name = "Reference_Features",
                        FilePath = "map_service_configs/GIBS/Reference_Features.xml",
                    },
                    {
                        Name = "Reference_Labels",
                        FilePath = "map_service_configs/GIBS/Reference_Labels.xml",
                    },
                    {
                        Type = "TileIndex",
                        Name = "Tile Indices",
                    },
                    {
                        Type = "SizeReference",
                        Name = "Size Reference",
                        Radii = earthEllipsoid,
                        BackgroundImagePath = "../arrows.png",
                    },
                    {
                        Name = "Ovation Prime",
                        Type = "Temporal",
                        FilePath = "map_service_configs/other/ovation_12_00.xml",
                        TilePixelSize = 64,
                        Enabled = true
                    }

                },
                HeightLayers = {
                    {
                        Name = "Terrain tileset",
                        FilePath = "map_service_configs/ESRI/TERRAIN.wms",
                        Enabled = true,
                        TilePixelSize = 64,
                    },
                },
            },
        }
    },
}
