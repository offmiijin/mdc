// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.lang.reflect.Field;
import java.util.Collection;
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Holder;
import net.minecraft.sounds.SoundEvent;
import net.minecraft.sounds.SoundSource;
import net.minecraft.world.damagesource.DamageSource;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.boss.enderdragon.EnderDragonPart;
import net.minecraft.world.flag.FeatureFlag;
import net.minecraft.world.level.ExplosionDamageCalculator;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.border.WorldBorder;
import net.minecraft.world.level.chunk.status.ChunkStatus;
import net.minecraft.world.level.entity.LevelEntityGetter;
import net.minecraft.world.level.saveddata.maps.MapId;
import net.minecraft.world.level.saveddata.maps.MapItemSavedData;
import net.minecraft.world.level.storage.LevelData;
import net.minecraft.world.level.timers.TimerQueue;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.CollisionContext;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * `Level` mínimo para o probe de colisão.
 *
 * O resolvedor do jogo ({@code Entity.collideBoundingBox}) exige um {@code Level}
 * — ele usa o nível para juntar as colisões do mundo às que passamos. Este stub
 * responde "nenhuma colisão do mundo", de modo que a única entrada é a lista de
 * shapes do caso. Assim o harness compara <b>a resolução do original</b>, não uma
 * reimplementação nossa.
 *
 * Instanciado com {@code Unsafe.allocateInstance}: o construtor de {@code Level}
 * pede dados de mundo que não existem fora de um servidor.
 *
 * Regras desta pasta (tools/oracle/README.md): {@code package dev.mdc.*}, e aqui
 * não há corpo de método do original — só stubs que devolvem vazio/nulo.
 */
public final class StubLevel extends Level {
    private StubLevel() { super(null, null, null, null, false, false, 0L, 0); }

    public static Level create() throws Exception {
        Field f = sun.misc.Unsafe.class.getDeclaredField("theUnsafe");
        f.setAccessible(true);
        sun.misc.Unsafe unsafe = (sun.misc.Unsafe) f.get(null);
        return (Level) unsafe.allocateInstance(StubLevel.class);
    }

    @Override public Iterable<VoxelShape> getBlockCollisionsFromContext(CollisionContext ctx, AABB box) { return List.of(); }
    @Override public Iterable<VoxelShape> getBlockCollisions(Entity e, AABB box) { return List.of(); }
    @Override public Iterable<VoxelShape> getCollisions(Entity e, AABB box) { return List.of(); }


    @Override public long nextSubTickCount() { return 0L; }
    @Override public net.minecraft.world.level.storage.LevelData getLevelData() { return null; }
    @Override public net.minecraft.server.MinecraftServer getServer() { return null; }
    @Override public net.minecraft.world.level.chunk.ChunkSource getChunkSource() { return null; }
    @Override public net.minecraft.util.RandomSource getRandom() { return null; }
    @Override public void playSound(Entity e, BlockPos p, SoundEvent s, SoundSource src, float v, float pi) {}
    @Override public void addParticle(net.minecraft.core.particles.ParticleOptions o, double x, double y, double z, double dx, double dy, double dz) {}
    @Override public net.minecraft.world.level.block.entity.BlockEntity getBlockEntity(BlockPos p) { return null; }
    @Override public BlockState getBlockState(BlockPos p) { return null; }
    @Override public net.minecraft.world.level.material.FluidState getFluidState(BlockPos p) { return null; }
    @Override public net.minecraft.world.level.chunk.ChunkAccess getChunk(int x, int z, ChunkStatus st, boolean load) { return null; }
    @Override public boolean hasChunk(int x, int z) { return false; }
    @Override public int getHeight(net.minecraft.world.level.levelgen.Heightmap.Types t, int x, int z) { return 0; }
    @Override public int getSkyDarken() { return 0; }
    @Override public net.minecraft.world.level.biome.BiomeManager getBiomeManager() { return null; }
    @Override public Holder<net.minecraft.world.level.biome.Biome> getUncachedNoiseBiome(int x, int y, int z) { return null; }
    @Override public boolean isClientSide() { return false; }
    @Override public int getSeaLevel() { return 0; }
    @Override public net.minecraft.world.level.dimension.DimensionType dimensionType() { return null; }
    @Override public net.minecraft.core.RegistryAccess registryAccess() { return null; }
    @Override public net.minecraft.world.flag.FeatureFlagSet enabledFeatures() { return null; }
    @Override public WorldBorder getWorldBorder() { return null; }
    @Override public net.minecraft.world.ticks.LevelTickAccess<net.minecraft.world.level.block.Block> getBlockTicks() { return null; }
    @Override public net.minecraft.world.ticks.LevelTickAccess<net.minecraft.world.level.material.Fluid> getFluidTicks() { return null; }
    @Override public <T> net.minecraft.world.ticks.ScheduledTick<T> createTick(BlockPos p, T type, int delay) { return null; }
    @Override public <T> net.minecraft.world.ticks.ScheduledTick<T> createTick(BlockPos p, T type, int delay, net.minecraft.world.ticks.TickPriority pr) { return null; }
    @Override public List<net.minecraft.world.entity.Entity> getEntities(Entity e, AABB box, java.util.function.Predicate<? super Entity> pr) { return List.of(); }
    @Override public <T extends Entity> List<T> getEntities(net.minecraft.world.level.entity.EntityTypeTest<Entity, T> t, AABB box, java.util.function.Predicate<? super T> pr) { return List.of(); }
    @Override public List<? extends net.minecraft.world.entity.player.Player> players() { return List.of(); }
    @Override public net.minecraft.world.level.BlockGetter getChunkForCollisions(int x, int z) { return null; }
    @Override public List<VoxelShape> getEntityCollisions(Entity e, AABB box) { return List.of(); }
    @Override public void levelEvent(Entity e, int type, BlockPos pos, int data) {}

    @Override public void gameEvent(Holder<net.minecraft.world.level.gameevent.GameEvent> e, net.minecraft.world.phys.Vec3 pos, net.minecraft.world.level.gameevent.GameEvent.Context ctx) {}

    @Override public void sendBlockUpdated(BlockPos p, BlockState a, BlockState b, int f) {}
    @Override public void playSeededSound(Entity e, double x, double y, double z, Holder<SoundEvent> s, SoundSource src, float v, float p, long seed) {}
    @Override public void playSeededSound(Entity e, Entity src, Holder<SoundEvent> s, SoundSource c, float v, float p, long seed) {}
    @Override public void explode(Entity e, DamageSource d, ExplosionDamageCalculator c, double x, double y, double z, float r, boolean fire, ExplosionInteraction i, net.minecraft.core.particles.ParticleOptions a, net.minecraft.core.particles.ParticleOptions b, net.minecraft.util.random.WeightedList<net.minecraft.core.particles.ExplosionParticleInfo> p, Holder<SoundEvent> s) {}
    @Override public String gatherChunkSourceStats() { return ""; }
    @Override public void setRespawnData(LevelData.RespawnData d) {}
    @Override public LevelData.RespawnData getRespawnData() { return null; }
    @Override public Entity getEntity(int id) { return null; }
    @Override public Collection<EnderDragonPart> dragonParts() { return List.of(); }
    @Override public net.minecraft.world.TickRateManager tickRateManager() { return null; }
    @Override public MapItemSavedData getMapData(MapId id) { return null; }
    @Override public void destroyBlockProgress(int id, BlockPos p, int d) {}
    @Override public net.minecraft.world.scores.Scoreboard getScoreboard() { return null; }
    @Override public net.minecraft.world.item.crafting.RecipeAccess recipeAccess() { return null; }
    @Override public LevelEntityGetter<Entity> getEntities() { return null; }
    @Override public net.minecraft.world.clock.ClockManager clockManager() { return null; }
    @Override public net.minecraft.world.attribute.EnvironmentAttributeSystem environmentAttributes() { return null; }
}
